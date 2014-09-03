/*
 * NPG-explorer, Nucleotide PanGenome explorer
 * Copyright (C) 2014 Boris Nagaev
 *
 * See the LICENSE file for terms of use.
 */

#include <fstream>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "AlignmentView.hpp"
#include "AlignmentModel.hpp"
#include "BlockSetWidget.hpp"
#include "BlockSet.hpp"
#include "bsa_algo.hpp"
#include "name_to_stream.hpp"

using namespace npge;

BlockSetPtr pangenome_bs;
BlockSetPtr genes_bs;
BlockSetPtr split_parts;
BlockSetPtr low_similarity;

typedef boost::shared_ptr<std::istream> IPtr;

MainWindow::MainWindow(int argc, char** argv,
                       QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    showMaximized();
    //
    pangenome_bs = new_bs();
    if (argc >= 2) {
        IPtr pangenome_file = name_to_istream(argv[1]);
        (*pangenome_file) >> (*pangenome_bs);
    } else {
        IPtr pangenome_file = name_to_istream("pangenome.bs");
        (*pangenome_file) >> (*pangenome_bs);
        //
        genes_bs = new_bs();
        genes_bs->add_sequences(pangenome_bs->seqs());
        IPtr genes_file = name_to_istream("features.bs");
        (*genes_file) >> (*genes_bs);
        //
        split_parts = new_bs();
        split_parts->add_sequences(pangenome_bs->seqs());
        IPtr split_file = name_to_istream("split.bs");
        (*split_file) >> (*split_parts);
        //
        low_similarity = new_bs();
        low_similarity->add_sequences(pangenome_bs->seqs());
        IPtr low_file = name_to_istream("low.bs");
        (*low_file) >> (*low_similarity);
        //
        IPtr test_bsaln = name_to_istream("pangenome.bsa");
        bsa_input(*pangenome_bs, *test_bsaln);
    }
    BlockSetWidget* bsw = new BlockSetWidget(pangenome_bs);
    bsw->set_genes(genes_bs);
    bsw->set_split_parts(split_parts);
    bsw->set_low_similarity(low_similarity);
    ui->verticalLayout_2->addWidget(bsw);
}

MainWindow::~MainWindow() {
    delete ui;
}

