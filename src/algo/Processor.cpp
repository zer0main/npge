/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2012-2016 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <map>
#include <typeinfo>
#include "boost-xtime.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/thread/mutex.hpp>

#include "Processor.hpp"
#include "BlockSet.hpp"
#include "FileWriter.hpp"
#include "class_name.hpp"
#include "string_arguments.hpp"
#include "throw_assert.hpp"
#include "Meta.hpp"
#include "Exception.hpp"
#include "name_to_stream.hpp"
#include "cast.hpp"
#include "Decimal.hpp"
#include "temp_file.hpp"
#include "global.hpp"

namespace npge {

struct BlockSetHolder {
public:
    BlockSetHolder():
        processor_(0) {
    }

    BlockSetPtr block_set() const {
        if (block_set_) {
            return block_set_;
        } else if (processor_) {
            return processor_->get_bs(name_);
        } else {
            block_set_ = new_bs();
            return block_set_;
        }
    }

    void set_block_set(BlockSetPtr block_set) {
        block_set_ = block_set;
        processor_ = 0;
        name_.clear();
    }

    void set_processor(Processor* processor, const std::string& name) {
        block_set_.reset();
        processor_ = processor;
        name_ = name;
    }

    std::string description_;

private:
    mutable BlockSetPtr block_set_;
    // or
    Processor* processor_;
    std::string name_;
};

typedef std::map<std::string, BlockSetHolder> BlockSetMap;

struct Option {
    Option():
        required_(false) {
    }

    Option(const std::string& name,
           const std::string& description,
           const AnyAs& default_value = AnyAs(),
           bool required = false):
        name_(name),
        description_(description),
        default_value_(default_value),
        required_(required) {
    }

    std::string name_;
    std::string description_;
    AnyAs default_value_;
    AnyAs value_;
    std::vector<Processor::OptionValidator> validators_;
    Processor::OptionGetter getter_;
    bool required_;

    const std::type_info& type() const {
        return default_value_.type();
    }
};

// option name to Option
typedef std::map<std::string, Option> Name2Option;

struct ProcessorImpl {
    ProcessorImpl():
        no_options_(false), milliseconds_(0),
        time_incrementers_(0),
        logged_(false), parent_(0), meta_(Meta::instance()),
        interrupted_(false) {
    }

    BlockSetMap map_;
    boost::mutex time_mutex_;
    boost::mutex tmp_files_mutex_;
    boost::posix_time::ptime before_;
    po::options_description ignored_options_;
    std::vector<Processor*> children_;
    Name2Option opts_;
    std::vector<Processor::OptionsChecker> checkers_;
    Strings tmp_files_;
    std::string name_;
    std::string key_;
    std::string opt_prefix_;
    Processor* parent_;
    Meta* meta_;
    mutable int milliseconds_;
    mutable int time_incrementers_;
    bool no_options_;
    bool interrupted_;
    bool logged_;
};

struct Processor::Impl : public ProcessorImpl {
};

TimeIncrementer::TimeIncrementer(const Processor* p):
    p_(p) {
    if (p_ && p_->timing()) {
        Processor::Impl* impl = p_->impl_;
        boost::mutex::scoped_lock lock(impl->time_mutex_);
        if (impl->time_incrementers_ == 0) {
            using namespace boost::posix_time;
            impl->before_ = microsec_clock::universal_time();
        }
        impl->time_incrementers_ += 1;
    }
}

TimeIncrementer::~TimeIncrementer() {
    if (p_ && p_->timing()) {
        Processor::Impl* impl = p_->impl_;
        boost::mutex::scoped_lock lock(impl->time_mutex_);
        impl->time_incrementers_ -= 1;
        if (impl->time_incrementers_ == 0) {
            using namespace boost::posix_time;
            ptime after = microsec_clock::universal_time();
            time_duration td = after - impl->before_;
            int msec = td.total_milliseconds();
            impl->milliseconds_ += msec;
        }
    }
}

static AnyAs workers_1(AnyAs workers) {
    int value = workers.as<int>();
    if (value == -1) {
        value = boost::thread::hardware_concurrency();
    }
    return value;
}

Processor::Processor() {
    impl_ = new Impl;
    add_gopt("workers", "number of threads", "WORKERS");
    add_opt_validator("workers", workers_1);
    add_gopt("timing", "measure time for each processor",
             "TIMING");
}

Processor::~Processor() {
    if (!impl_->logged_ && impl_->milliseconds_) {
        std::stringstream ss;
        log_processor(ss, 0);
        write_log(ss.str());
    }
    BOOST_FOREACH (Processor* child, impl_->children_) {
        child->impl_->parent_ = 0;
        delete child;
    }
    impl_->children_.clear();
    if (!go("NPGE_DEBUG", false).as<bool>()) {
        boost::mutex::scoped_lock lock(impl_->tmp_files_mutex_);
        BOOST_FOREACH (const std::string& tmp, impl_->tmp_files_) {
            remove_file(tmp);
        }
    }
    if (parent()) {
        set_parent(0);
    }
    delete impl_;
}

void Processor::declare_bs(const std::string& name,
                           const std::string& description) {
    impl_->map_[name].description_ = description;
}

void Processor::remove_bs(const std::string& name) {
    impl_->map_.erase(name);
}

std::string Processor::bs_description(const std::string& name) const {
    BlockSetMap::const_iterator it = impl_->map_.find(name);
    if (it == impl_->map_.end()) {
        return "";
    } else {
        return it->second.description_;
    }
}

BlockSetPtr Processor::get_bs(const std::string& name) const {
    BlockSetMap::const_iterator it = impl_->map_.find(name);
    if (it == impl_->map_.end()) {
        BlockSetPtr bs = new_bs();
        impl_->map_[name].set_block_set(bs);
        return bs;
    } else {
        return it->second.block_set();
    }
}

void Processor::set_bs(const std::string& name, BlockSetPtr bs) {
    impl_->map_[name].set_block_set(bs);
}

bool Processor::has_bs(const std::string& name) const {
    BlockSetMap::const_iterator it = impl_->map_.find(name);
    return (it != impl_->map_.end());
}

void Processor::point_bs(const std::string& mapping, Processor* processor) {
    size_t eq_pos = mapping.find("=");
    ASSERT_MSG(eq_pos != std::string::npos,
               ("Bad mapping: " + mapping).c_str());
    std::string name_in_this = mapping.substr(0, eq_pos);
    std::string name_in_processor = mapping.substr(eq_pos + 1);
    ASSERT_MSG(processor != this || name_in_this != name_in_processor,
               ("Trying to set self-pointed blockset: " + mapping +
                " in processor " + key()).c_str());
    impl_->map_[name_in_this].set_processor(processor, name_in_processor);
}

void Processor::set_options(const std::string& options, Processor* processor) {
    if (!processor) {
        processor = parent();
    }
    if (processor) {
        point_bs("target=target", processor);
        point_bs("other=other", processor);
    }
    bool nooptions = false;
    Strings ignored;
    Strings default_opts;
    using namespace boost::algorithm;
    using boost::tokenizer;
    using boost::escaped_list_separator;
    typedef tokenizer<escaped_list_separator<char> > tok_t;
    tok_t tok(options, escaped_list_separator<char>('\\', ' ', '\"'));
    BOOST_FOREACH (std::string opt, tok) {
        trim_right(opt);
        size_t eq_pos = opt.find('=');
        if (eq_pos != std::string::npos) {
            if (opt[0] == '-') {
                // command line option
                std::string opt_name = opt.substr(0, eq_pos);
                std::string opt_value = opt.substr(eq_pos + 1);
                bool ignore = opt_name[opt_name.size() - 1] == ':';
                if (ignore) {
                    opt_name.resize(opt_name.size() - 1);
                }
                default_opts.push_back(opt_name);
                default_opts.push_back(opt_value);
                if (ignore) {
                    std::string short_name;
                    if (opt_name.size() > 1 && opt_name[1] == '-') {
                        // option like --workers
                        short_name = opt_name.substr(2, opt_name.size() - 2);
                    } else {
                        // option like -i
                        short_name = opt_name.substr(1, opt_name.size() - 1);
                    }
                    ignored.push_back(short_name);
                }
            } else if (processor) {
                point_bs(opt, processor);
            } else {
                // TODO bad option
            }
        } else if (opt == "no_options") {
            nooptions = true;
        } else if (starts_with(opt, "prefix|")) {
            int sep = opt.find('|');
            std::string prefix_value = opt.substr(sep + 1);
            set_opt_prefix(prefix_value);
        } else {
            // TODO bad option
        }
    }
    if (!default_opts.empty()) {
        apply_vector_options(default_opts);
    }
    BOOST_FOREACH (const std::string& opt, ignored) {
        add_ignored_option(opt);
    }
    if (nooptions) {
        set_no_options(true);
    }
}

BlockSetPtr Processor::block_set() const {
    return get_bs("target");
}

void Processor::set_block_set(BlockSetPtr block_set) {
    set_bs("target", block_set);
}

BlockSetPtr Processor::other() const {
    return get_bs("other");
}

void Processor::set_other(BlockSetPtr other) {
    set_bs("other", other);
}

void Processor::set_empty_block_set() {
    set_block_set(new_bs());
}

void Processor::set_empty_other() {
    set_other(new_bs());
}

void Processor::get_block_sets(Strings& block_sets) const {
    BOOST_FOREACH (const BlockSetMap::value_type& name_and_bs, impl_->map_) {
        const std::string& name = name_and_bs.first;
        block_sets.push_back(name);
    }
}

int Processor::workers() const {
    return opt_value("workers").as<int>();
}

void Processor::set_workers(int workers) {
    set_opt_value("workers", workers);
}

typedef boost::shared_ptr<std::ostream> SharedStream;
typedef std::map<std::string, SharedStream> Omap;
static Omap log_omap_;
static boost::mutex log_omap_mutex_;

// See http://stackoverflow.com/a/3854549
static std::string currentTimeString() {
    using namespace boost::posix_time;
    typedef boost::date_time::c_local_adjustor<ptime> adj;
    const ptime utc_now = second_clock::universal_time();
    const ptime now = adj::utc_to_local(utc_now);
    time_duration offset = now - utc_now;
    std::stringstream out;
    out << to_simple_string(now);
    out << " UTC";
    if (offset.is_negative()) {
        out << "-";
        offset = offset.invert_sign();
    } else {
        out << "+";
    }
    out.width(2);
    out.fill('0');
    out << offset.hours() << ':';
    out.width(2);
    out.fill('0');
    out << offset.minutes();
    return out.str();
}

void Processor::write_log(const std::string& m) const {
    std::string time = '[' + currentTimeString() + ']';
    std::string line = time + ' ' + key() + ' ' + m;
    std::string log_to = go("LOG_TO").as<std::string>();
    boost::mutex::scoped_lock lock(log_omap_mutex_);
    Omap::iterator it = log_omap_.find(log_to);
    if (it == log_omap_.end()) {
        SharedStream stream = name_to_ostream(log_to);
        log_omap_[log_to] = stream;
        (*stream) << line << "\n";
    } else {
        SharedStream& stream = it->second;
        (*stream) << line << "\n";
    }
}

void Processor::close_log() const {
    boost::mutex::scoped_lock lock(log_omap_mutex_);
    std::string log_to = go("LOG_TO").as<std::string>();
    log_omap_.erase(log_to);
}

bool Processor::no_options() const {
    return impl_->no_options_;
}

void Processor::set_no_options(bool no_options) {
    impl_->no_options_ = no_options;
}

void Processor::add_ignored_option(const std::string& option) {
    add_unique_options(impl_->ignored_options_)(option.c_str(), "");
}

bool Processor::is_ignored(const std::string& option) const {
    const Processor* p = this;
    std::string name = option;
    while (p) {
        name = p->opt_prefix() + name;
        if (p->impl_->ignored_options_.find_nothrow(name, /* approx */ false)) {
            return true;
        }
        p = p->parent();
    }
    return false;
}

bool Processor::timing() const {
    return opt_value("timing").as<bool>();
}

void Processor::set_timing(bool timing) {
    set_opt_value("timing", timing);
}

void Processor::assign(const Processor& other) {
    set_block_set(other.block_set());
    set_other(other.other());
    set_workers(other.workers());
    set_timing(other.timing());
}

static void add_option(po::options_description& desc, const std::string name,
                       const Option& opt, const AnyAs& value) {
    ASSERT_MSG(good_opt_type(opt.type()),
               ("Bad type of option " + name).c_str());
    typedef boost::shared_ptr<po::option_description> OptPtr;
    po::value_semantic* vs = 0;
    if (opt.type() == typeid(int)) {
        vs = po::value<int>()->default_value(value.as<int>());
    } else if (opt.type() == typeid(bool)) {
        vs = po::value<bool>()->default_value(value.as<bool>());
    } else if (opt.type() == typeid(Decimal)) {
        vs = po::value<std::string>()
             ->default_value(value.as<Decimal>().to_s());
    } else if (opt.type() == typeid(std::string)) {
        po::typed_value<std::string>* tv = po::value<std::string>();
        tv->default_value(value.as<std::string>());
        if (opt.required_) {
            tv->required();
        }
        vs = tv;
    } else if (opt.type() == typeid(Strings)) {
        po::typed_value<Strings >* tv;
        tv = po::value<Strings>()->multitoken();
        Strings list = value.as<Strings>();
        using namespace boost::algorithm;
        std::string list_str = join(list, " ");
        tv->default_value(list, list_str);
        if (opt.required_) {
            tv->required();
        }
        vs = tv;
    }
    ASSERT_TRUE(vs);
    add_unique_options(desc)(name.c_str(), vs, opt.description_.c_str());
}

void Processor::add_options(po::options_description& desc) const {
    check_interruption();
    if (!no_options()) {
        // add self options
        po::options_description self_options_1;
        typedef Name2Option::value_type Pair;
        BOOST_FOREACH (const Pair& name_and_opt, impl_->opts_) {
            const Option& opt = name_and_opt.second;
            if (!is_ignored(opt.name_)) {
                add_option(self_options_1, opt_prefixed(opt.name_),
                           opt, opt_value(opt.name_));
            }
        }
        po::options_description self_options_2;
        add_options_impl(self_options_2);
        po::options_description self_options_2a;
        copy_not_ignored(self_options_2, self_options_2a);
        po::options_description new_opts(name());
        add_new_options(self_options_1, new_opts, &desc);
        add_new_options(self_options_2a, new_opts, &desc);
        if (!new_opts.options().empty()) {
            desc.add(new_opts);
        }
        BOOST_FOREACH (const Processor* child, impl_->children_) {
            child->add_options(desc);
        }
    }
}

void Processor::apply_options(const po::variables_map& vm0) {
    check_interruption();
    po::variables_map vm = vm0;
    if (no_options()) {
        // remove all options except --timing and --workers
        BOOST_FOREACH (const po::variables_map::value_type& key_value, vm0) {
            const std::string& key = key_value.first;
            if (key != opt_prefixed("timing") &&
                    key != opt_prefixed("workers")) {
                vm.erase(key);
            }
        }
    }
    // remove ignored options (even --timing and --workers can be ignored)
    typedef boost::shared_ptr<po::option_description> OptPtr;
    BOOST_FOREACH (OptPtr ignored_opt, impl_->ignored_options_.options()) {
        vm.erase(ignored_opt->long_name());
    }
    typedef Name2Option::value_type Pair;
    BOOST_FOREACH (const Pair& name_and_opt, impl_->opts_) {
        const std::string& name = name_and_opt.first;
        const Option& opt = name_and_opt.second;
        std::string prefixed_name = opt_prefixed(name);
        if (vm.count(prefixed_name)) {
            AnyAs value = vm[prefixed_name].value();
            if (opt.type() == typeid(Decimal)) {
                value = Decimal(value.as<std::string>());
            }
            set_opt_value(name, value);
        }
    }
    apply_options_impl(vm);
    BOOST_FOREACH (Processor* child, impl_->children_) {
        child->apply_options(vm);
    }
}

Strings Processor::options_errors() const {
    Strings result;
    typedef Name2Option::value_type Pair;
    BOOST_FOREACH (const Pair& name_and_opt, impl_->opts_) {
        const std::string& name = name_and_opt.first;
        const Option& opt = name_and_opt.second;
        if (opt.required_) {
            if (opt.type() == typeid(std::string)) {
                if (opt_value(name).as<std::string>().empty()) {
                    result.push_back("Required option " + opt.name_ +
                                     " is empty");
                }
            }
            if (opt.type() == typeid(Strings)) {
                if (opt_value(name).as<Strings >().empty()) {
                    result.push_back("Required option " + opt.name_ +
                                     " is empty");
                }
            }
        }
    }
    BOOST_FOREACH (const OptionsChecker& checker, impl_->checkers_) {
        std::string message;
        bool valid = checker(message);
        if (!valid) {
            result.push_back(message);
        }
    }
    BOOST_FOREACH (Processor* child, impl_->children_) {
        BOOST_FOREACH (const std::string& e, child->options_errors()) {
            result.push_back(e);
        }
    }
    return result;
}

Strings Processor::options_warnings() const {
    Strings result;
    BOOST_FOREACH (const OptionsChecker& checker, impl_->checkers_) {
        std::string message;
        bool valid = checker(message);
        if (valid && !message.empty()) {
            result.push_back(message);
        }
    }
    return result;
}

void Processor::apply_vector_options(const Strings& options) {
    StringToArgv args;
    std::string prev = "";
    typedef std::map<std::string, std::string> S2S;
    S2S replaced;
    BOOST_FOREACH (std::string opt, options) {
        using namespace boost::algorithm;
        if (starts_with(opt, "$")) {
            replaced[prev] = opt;
            // replace "$OPT" with current values
            opt = go(opt.substr(1)).to_s();
        }
        args.add_argument(opt);
        prev = opt;
    }
    po::options_description desc;
    add_options(desc);
    po::variables_map vm;
    po::store(po::command_line_parser(args.argc(), args.argv()).options(desc)
              .allow_unregistered().run(), vm);
    // po::notify(vm); // to pass required options check
    BOOST_FOREACH (const S2S::value_type& s2s, replaced) {
        // set "$OPT" back
        typedef po::variable_value VV;
        VV& vv = const_cast<VV&>(vm[s2s.first]);
        vv.value() = s2s.second;
    }
    apply_options(vm);
}

void Processor::apply_string_options(const std::string& options) {
    using namespace boost::algorithm;
    using boost::tokenizer;
    using boost::escaped_list_separator;
    typedef tokenizer<escaped_list_separator<char> > tok_t;
    tok_t tok(options, escaped_list_separator<char>('\\', ' ', '\"'));
    Strings opts;
    BOOST_FOREACH (std::string opt, tok) {
        trim_right(opt);
        opts.push_back(opt);
    }
    apply_vector_options(opts);
}

void Processor::run() const {
    TimeIncrementer ti(this);
    check_interruption();
    Strings errors = options_errors();
    if (!errors.empty()) {
        using namespace boost::algorithm;
        throw Exception("Errors in " + key() + "'s options: " +
                        join(errors, ", "));
    }
    bool timing1 = timing();
    if (timing1) {
        write_log("begin");
        // it is important to call key() to memorize value.
        // RTTI would be invalid in ~Processor()
    }
    if (workers() != 0 && block_set()) {
        run_impl();
    }
    if (timing1) {
        write_log("end");
    }
}

void Processor::apply_to_block(Block* block) const {
    apply_to_block_impl(block);
}

std::string Processor::name() const {
    if (!impl_->name_.empty()) {
        return impl_->name_;
    } else {
        const char* ni = name_impl();
        if (*ni == '\0') {
            // empty
            return key();
        } else {
            return ni;
        }
    }
}

void Processor::set_name(const std::string& name) {
    impl_->name_ = name;
}

void Processor::apply(const BlockSetPtr& bs) const {
    BlockSetPtr prev = block_set();
    const_cast<Processor*>(this)->set_block_set(bs);
    run();
    const_cast<Processor*>(this)->set_block_set(prev);
}

std::string Processor::key() const {
    if (impl_->key_.empty()) {
        impl_->key_ = processor_name(this);
    }
    return impl_->key_;
}

void Processor::set_key(const std::string& key) {
    impl_->key_ = key;
}

Processor* Processor::parent() const {
    return impl_->parent_;
}

static void remove_child(std::vector<Processor*>& children, Processor* child) {
    children.erase(std::remove(children.begin(), children.end(), child),
                   children.end()); // TODO template remove_from_vector()
}

void Processor::set_parent(Processor* parent) {
    if (parent != impl_->parent_) {
        if (impl_->parent_) {
            remove_child(impl_->parent_->impl_->children_, this);
        }
        impl_->parent_ = parent;
        if (parent) {
            parent->impl_->children_.push_back(this);
        }
    }
}

std::vector<Processor*> Processor::children() const {
    return impl_->children_;
}

Processor* Processor::clone() const {
    ASSERT_TRUE(meta());
    Processor* result = meta()->get_plain(key());
    result->impl_->map_ = impl_->map_;
    result->impl_->no_options_ = impl_->no_options_;
    result->impl_->name_ = impl_->name_;
    add_new_options(impl_->ignored_options_,
                    result->impl_->ignored_options_);
    result->impl_->key_ = impl_->key_;
    result->set_parent(parent());
    result->impl_->opt_prefix_ = impl_->opt_prefix_;
    result->impl_->opts_ = impl_->opts_;
    return result;
}

Meta* Processor::meta() const {
    if (impl_->meta_) {
        return impl_->meta_;
    } else if (parent()) {
        return parent()->meta();
    } else {
        return Meta::instance();
    }
}

void Processor::set_meta(Meta* meta) {
    impl_->meta_ = meta;
}

AnyAs Processor::go(const std::string& key,
                    const AnyAs& dflt) const {
    ASSERT_TRUE(meta());
    return meta()->get_opt(key, dflt);
}

const std::string& Processor::opt_prefix() const {
    return impl_->opt_prefix_;
}

void Processor::set_opt_prefix(const std::string& opt_prefix) {
    impl_->opt_prefix_ = opt_prefix;
}

std::string Processor::opt_prefixed(const std::string& name) const {
    std::vector<const Processor*> ancestors;
    const Processor* processor = this;
    while (processor) {
        ancestors.push_back(processor);
        processor = processor->parent();
    }
    std::string result;
    BOOST_REVERSE_FOREACH (const Processor* p, ancestors) {
        result += p->opt_prefix();
    }
    result += name;
    return result;
}

Strings Processor::opts() const {
    Strings result;
    typedef Name2Option::value_type Pair;
    BOOST_FOREACH (const Pair& name_and_opt, impl_->opts_) {
        const Option& opt = name_and_opt.second;
        result.push_back(opt.name_);
    }
    return result;
}

bool Processor::has_opt(const std::string& name) const {
    return impl_->opts_.find(name) != impl_->opts_.end();
}

const std::string& Processor::opt_description(const std::string& name) const {
    typedef Name2Option::const_iterator It;
    It it = impl_->opts_.find(name);
    if (it == impl_->opts_.end()) {
        throw Exception("No option with name '" + name + "'");
    } else {
        return it->second.description_;
    }
}

const std::type_info& Processor::opt_type(const std::string& name) const {
    typedef Name2Option::const_iterator It;
    It it = impl_->opts_.find(name);
    if (it == impl_->opts_.end()) {
        throw Exception("No option with name '" + name + "'");
    } else {
        return it->second.type();
    }
}

const AnyAs& Processor::default_opt_value(const std::string& name) const {
    typedef Name2Option::const_iterator It;
    It it = impl_->opts_.find(name);
    if (it == impl_->opts_.end()) {
        throw Exception("No option with name '" + name + "'");
    } else {
        return it->second.default_value_;
    }
}

AnyAs Processor::opt_value(const std::string& name) const {
    typedef Name2Option::const_iterator It;
    It it = impl_->opts_.find(name);
    if (it == impl_->opts_.end()) {
        throw Exception("No option with name '" + name + "'");
    }
    const Option& opt = it->second;
    if (!opt.value_.empty()) {
        return opt.value_;
    }
    if (!opt.getter_.empty()) {
        AnyAs result = opt.getter_();
        if (result.type() == typeid(std::string) &&
                opt.type() == typeid(Strings)) {
            Strings vector;
            vector.push_back(result.as<std::string>());
            result = vector;
        }
        ASSERT_MSG(result.type() == opt.type(),
                   (TO_S(result.type().name()) + " != " +
                    TO_S(opt.type().name())).c_str());
        return result;
    }
    return opt.default_value_;
}

static AnyAs get_go(Processor* p,
                    ProcessorImpl* impl,
                    const std::string& name,
                    const std::string& key) {
    AnyAs v = p->go(key);
    typedef Name2Option::iterator It;
    It it = impl->opts_.find(name);
    if (it == impl->opts_.end()) {
        throw Exception("No option with name '" + name + "'");
    }
    Option& opt = it->second;
    BOOST_FOREACH (const Processor::OptionValidator& validator,
                  opt.validators_) {
        v = validator(v);
    }
    return v;
}

void Processor::set_opt_value(const std::string& name,
                              const AnyAs& value) {
    typedef Name2Option::iterator It;
    It it = impl_->opts_.find(name);
    if (it == impl_->opts_.end()) {
        throw Exception("No option with name '" + name + "'");
    }
    Option& opt = it->second;
    if (value.type() == typeid(std::string)) {
        std::string str = value.as<std::string>();
        if (!str.empty() && str[0] == '$') {
            opt.value_ = AnyAs();
            set_opt_getter(name, boost::bind(get_go,
                                             this,
                                             impl_,
                                             name,
                                             str.substr(1)));
            return;
        }
    }
    AnyAs v = value;
    if (v.type() == typeid(std::string) &&
            opt.type() == typeid(Strings)) {
        Strings vector;
        vector.push_back(v.as<std::string>());
        v = vector;
    }
    BOOST_FOREACH (const OptionValidator& validator, opt.validators_) {
        v = validator(v);
    }
    if (!v.empty() && opt.type() != v.type()) {
        throw Exception("Type of value of option "
                        "'" + name + "' (" + v.type().name() + ") "
                        "differs from type of default value "
                        "(" + opt.type().name() + ")");
    }
    if (!any_equal(v, opt.default_value_) || !opt.value_.empty()) {
        opt.value_ = v;
    }
}

void Processor::set_opt_getter(const std::string& name,
                               const OptionGetter& getter) {
    typedef Name2Option::iterator It;
    It it = impl_->opts_.find(name);
    if (it == impl_->opts_.end()) {
        throw Exception("No option with name '" + name + "'");
    }
    Option& opt = it->second;
    opt.getter_ = getter;
}

void Processor::fix_opt_value(const std::string& name,
                              const AnyAs& value) {
    set_opt_value(name, value);
    add_ignored_option(opt_prefixed(name));
}

void Processor::fix_opt_getter(const std::string& name,
                               const OptionGetter& getter) {
    set_opt_getter(name, getter);
    add_ignored_option(opt_prefixed(name));
}

void Processor::interrupt() {
    impl_->interrupted_ = true;
}

bool Processor::is_interrupted() const {
    const Processor* p = this;
    while (p) {
        if (p->impl_->interrupted_) {
            return true;
        }
        p = p->parent();
    }
    return false;
}

std::string Processor::escape_backslash(const std::string& str) {
    return escape_path(str);
}

std::string Processor::tmp_file() const {
    std::string tmp = temp_file();
    boost::mutex::scoped_lock lock(impl_->tmp_files_mutex_);
    impl_->tmp_files_.push_back(tmp);
    return tmp;
}

void Processor::add_options_impl(po::options_description& desc) const {
}

void Processor::apply_options_impl(const po::variables_map& vm) {
}

void Processor::run_impl() const {
}

struct BlockDetacher {
    BlockDetacher(BlockSet& bs, Block* block):
        bs_(bs), block_(block) {
    }

    ~BlockDetacher() {
        bs_.detach(block_);
    }

    BlockSet& bs_;
    Block* block_;
};

void Processor::apply_to_block_impl(Block* block) const {
    BlockSetPtr block_set = new_bs();
    block_set->insert(block);
    BlockDetacher bd(*block_set, block);
    apply(block_set);
}

const char* Processor::name_impl() const {
    return "";
}

void Processor::check_interruption() const {
    const Processor* p = this;
    while (p) {
        if (p->impl_->interrupted_) {
            p->impl_->interrupted_ = false;
            throw Exception(p->key() + " was interrupted");
        }
        p = p->parent();
    }
}

void Processor::add_gopt(const std::string& name,
                         const std::string& description,
                         const std::string& global_opt_name,
                         bool required) {
    AnyAs default_value = go(global_opt_name);
    add_opt(name, description, default_value, required);
    set_opt_value(name, "$" + global_opt_name);
}

void Processor::add_opt(const std::string& name,
                        const std::string& description,
                        const AnyAs& default_value,
                        bool required) {
    ASSERT_MSG(good_opt_type(default_value.type()),
               ("Bad type of option " + name).c_str());
    impl_->opts_[name] = Option(name, description, default_value, required);
}

void Processor::remove_opt(const std::string& name, bool apply_prefix) {
    impl_->opts_.erase(apply_prefix ? opt_prefixed(name) : name);
}

void Processor::add_opt_validator(const std::string& name,
                                  const OptionValidator& validator) {
    ASSERT_TRUE(has_opt(name));
    Option& opt = impl_->opts_[name];
    AnyAs v = validator(opt.default_value_);
    ASSERT_EQ(v.type_name(), opt.default_value_.type_name());
    impl_->opts_[name].validators_.push_back(validator);
}

void Processor::add_opt_check(const OptionsChecker& checker) {
    impl_->checkers_.push_back(checker);
}

static Decimal decimal_option(const Processor* p,
                              const std::string& name) {
    if (p->opt_type(name) == typeid(Decimal)) {
        return p->opt_value(name).as<Decimal>();
    } else if (p->opt_type(name) == typeid(int)) {
        return Decimal(p->opt_value(name).as<int>());
    } else {
        throw Exception("Bad option type (" +
                        std::string(p->opt_type(name).name()) +
                        "), must be int or Decimal");
    }
}

static Decimal decimal_transparent(Decimal value) {
    return value;
}

static bool g_checker(bool result,
                      Decimal left, Decimal right,
                      const std::string& s,
                      std::string& d) {
    if (!result) {
        d = s + " (are: " + left.to_s() +
            ", " + right.to_s() + ")";
    }
    return result;
}

static void check_opt(const std::string& opt_name,
                      Name2Option& opts) {
    const std::type_info& t = opts[opt_name].type();
    if (t != typeid(int) && t != typeid(Decimal)) {
        throw Exception("Option type for rule must be int "
                        "or Decimal, not " +
                        std::string(t.name()) +
                        " (option " + opt_name + ")");
    }
}

void Processor::add_opt_rule(const std::string& rule,
                             const std::string& message) {
    using namespace boost::algorithm;
    Strings parts;
    split(parts, rule, isspace, token_compress_on);
    const std::string& left_opt = parts[0];
    const std::string& op = parts[1];
    const std::string& right = parts[2];
    if (!has_opt(left_opt)) {
        throw Exception("No such option: " + left_opt);
    }
    check_opt(left_opt, impl_->opts_);
    typedef boost::function<Decimal()> Getter;
    Getter left_getter = boost::bind(decimal_option,
                                     this, left_opt);
    boost::function<bool(Decimal, Decimal)> cmp;
    if (op == "<") {
        cmp = std::less<Decimal>();
    } else if (op == ">") {
        cmp = std::greater<Decimal>();
    } else if (op == "<=") {
        cmp = std::less_equal<Decimal>();
    } else if (op == ">=") {
        cmp = std::greater_equal<Decimal>();
    } else {
        throw Exception("Operators for rule must be "
                        "<.>,<=,>=, not " + op);
    }
    Getter right_getter;
    if (has_opt(right)) {
        check_opt(right, impl_->opts_);
        right_getter = boost::bind(decimal_option,
                                   this, right);
    } else {
        // may throw here if bad value
        Decimal right_value(right);
        right_getter = boost::bind(decimal_transparent,
                                   right_value);
    }
    add_opt_check(boost::bind(g_checker, boost::bind(cmp,
                              boost::bind(left_getter),
                              boost::bind(right_getter)),
                              boost::bind(left_getter),
                              boost::bind(right_getter),
                              message, _1));
}

void Processor::add_opt_rule(const std::string& rule) {
    add_opt_rule(rule, rule);
}

void Processor::log_processor(std::ostream& o, int depth) {
    using namespace boost::posix_time;
    impl_->logged_ = true;
    if (!parent()) {
        o << std::endl;
    }
    const int TAB_SIZE = 4;
    o << std::string(depth * TAB_SIZE, ' '); // indent
    o << key() + ": ";
    o << to_simple_string(milliseconds(impl_->milliseconds_));
    o << std::endl;
    BOOST_FOREACH (Processor* child, impl_->children_) {
        child->log_processor(o, depth + 1);
    }
}

void Processor::copy_not_ignored(const po::options_description& source,
                                 po::options_description& dest) const {
    typedef boost::shared_ptr<po::option_description> OptPtr;
    BOOST_FOREACH (OptPtr opt, source.options()) {
        bool good_option = true;
        const Processor* p = this;
        while (p) {
            if (p->impl_->ignored_options_.find_nothrow(opt->long_name(),
                    /* approx */ false)) {
                good_option = false;
                break;
            }
            p = p->parent();
        }
        if (good_option) {
            dest.add(opt);
        }
    }
}

std::string processor_name(const Processor* processor) {
    return class_name(typeid(*processor).name());
}

std::ostream& operator<<(std::ostream& o,
                         const Processor& p) {
    o << p.key();
    return o;
}

}

