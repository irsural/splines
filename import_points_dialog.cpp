#include "import_points_dialog.h"
#include "ui_import_points_form.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <algorithm>


import_points_dialog_t::import_points_dialog_t(std::vector<double> a_correct_points, QStandardItemModel *a_csv_model,
  QString a_filepath, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::Dialog),
  m_csv_filepath(a_filepath),
  m_csv_model(a_csv_model)
{
  ui->setupUi(this);

  //Разрешаем вводить только цифры и ;
  QRegExp filter_correct_points("^[0-9.;]+$");
  ui->correct_points_edit->setValidator(
    new QRegExpValidator(filter_correct_points, this));

  //Для выделения столбцов и строк
  QHeaderView* hh = ui->points_table->horizontalHeader();
  connect(hh, &QHeaderView::sectionClicked,
    this, &import_points_dialog_t::hhSelected);
  QHeaderView* vh = ui->points_table->verticalHeader();
  connect(vh, &QHeaderView::sectionClicked,
    this, &import_points_dialog_t::vhSelected);

  ui->points_table->setModel(m_csv_model);

  if (m_csv_filepath != "") {
    ui->filepath_edit->setText(m_csv_filepath);
  }

  QString points_str = "";
  for (auto point: a_correct_points) {
    points_str += QString::number(point) + ";";
  }
  ui->correct_points_edit->setText(points_str);
}

import_points_dialog_t::~import_points_dialog_t()
{
  delete ui;
}

void import_points_dialog_t::on_choose_file_button_clicked()
{
  m_csv_filepath = QFileDialog::getOpenFileName(this, tr("Open .csv file"),
    m_csv_filepath, tr("CSV Files(*.csv)"));

  if (m_csv_filepath != "") {
    ui->filepath_edit->setText(m_csv_filepath);
    emit filename_changed(m_csv_filepath);
    on_import_button_clicked();
  }
}

void import_points_dialog_t::on_import_button_clicked()
{
  m_csv_filepath = ui->filepath_edit->text();
  QFile file(m_csv_filepath);
  if (file.open(QFile::ReadOnly | QFile::Text)) {
    insert_points_to_table(&file);
    file.close();
  } else {
    QMessageBox::critical(this, "Error", "Can't open file");
  }
}

void import_points_dialog_t::hhSelected(int a_column)
{
  if (a_column != 0) {
    ui->points_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->points_table->setSelectionBehavior(QAbstractItemView::SelectColumns);
    ui->points_table->selectColumn(a_column);
    emit selected_col_changed(a_column);
  }
}

void import_points_dialog_t::vhSelected(int a_row)
{
  if (a_row != 0) {
    ui->points_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->points_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->points_table->selectRow(a_row);
    emit selected_row_changed(a_row);
  }
}

void import_points_dialog_t::resize_dialog_for_table()
{
  QSize new_table_size(0, 0);
  for (int i = 0; i < m_csv_model->columnCount(); i++)
    new_table_size.setWidth(new_table_size.width() + ui->points_table->columnWidth(i));
  for (int i = 0; i < m_csv_model->rowCount(); i++)
    new_table_size.setHeight(new_table_size.height() + ui->points_table->rowHeight(i));

  QSize win_frames_size = this->size() - ui->points_table->size();

  if (ui->points_table->width() < new_table_size.width() &&
      ui->points_table->height() < new_table_size.height())
  {
//    ui->points_table->resize(new_table_size);
//    this->resize(win_frames_size + new_table_size + QSize(50, 50));
  }
}

void import_points_dialog_t::insert_points_to_table(QFile* a_file)
{
  m_csv_model->clear();

  QTextStream file_sream(a_file);
  while (!file_sream.atEnd())
  {
    QString line = file_sream.readLine();
    QList<QStandardItem *> standardItemsList;
    for (QString item : line.split(";")) {
        standardItemsList.append(new QStandardItem(item));
    }
    m_csv_model->insertRow(m_csv_model->rowCount(), standardItemsList);
  }
  ui->points_table->resizeRowsToContents();
  ui->points_table->resizeColumnsToContents();

  resize_dialog_for_table();
}

std::vector<double> import_points_dialog_t::parse_correct_points()
{
  std::vector<double> correct_points;

  QString correct_points_str = ui->correct_points_edit->text();
  if (correct_points_str != "") {
    QStringList points_str_list = correct_points_str.split(";",
      QString::SkipEmptyParts);
    for (auto str_point: points_str_list) {
      correct_points.push_back(str_point.toDouble());
    }
   }
  return correct_points;
}

void import_points_dialog_t::on_accept_points_button_clicked()
{
  bool rows_selected = !ui->points_table->selectionModel()->
    selectedRows().isEmpty();
  bool cols_selected = !ui->points_table->selectionModel()->
    selectedColumns().isEmpty();
  //Одновременно только или строки или столбцы
  assert(!rows_selected || !cols_selected);

  if (rows_selected || cols_selected) {
    assert(rows_selected == !cols_selected);

    std::vector<double>&& correct_points = parse_correct_points();
    emit selection_done(correct_points);
    close();

  } else {
    QMessageBox::critical(this, "Error", "Select cells to continue");
  }
}

void import_points_dialog_t::on_cancel_button_clicked()
{
  close();
}

