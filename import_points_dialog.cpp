#include "import_points_dialog.h"
#include "ui_import_points_form.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <algorithm>
#include <memory>


import_points_dialog_t::import_points_dialog_t(const std::vector<double> &a_correct_points,
  QStandardItemModel *a_csv_model, QString a_filepath, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::Dialog),
  m_csv_filepath(a_filepath),
  m_csv_model(a_csv_model),
  m_data_format_error(false)
{
  ui->setupUi(this);

  //Разрешаем вводить только цифры и ;
  QRegExp filter_correct_points("^[0-9.;]+$");
  ui->correct_points_edit->setValidator(new QRegExpValidator(filter_correct_points, this));

  //Для выделения только столбцов
  QHeaderView* hh = ui->points_table->horizontalHeader();
  connect(hh, &QHeaderView::sectionClicked, this, &import_points_dialog_t::hhSelected);
  //Для выделения только строк
  QHeaderView* vh = ui->points_table->verticalHeader();
  connect(vh, &QHeaderView::sectionClicked, this, &import_points_dialog_t::vhSelected);

  ui->points_table->setModel(m_csv_model);
  ui->filepath_edit->setText(m_csv_filepath);

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

void import_points_dialog_t::insert_points_to_table(QFile* a_file)
{
  m_csv_model->clear();
  m_data_format_error = false;

  QTextStream file_sream(a_file);
  QString header = file_sream.readLine();
  int columns_count = header.split(";").size();
  file_sream.seek(0);

  while (!file_sream.atEnd())
  {
    auto values = file_sream.readLine().split(";");
    if (values.size() != columns_count) {
      m_data_format_error = true;
    }

    QList<QStandardItem*> standardItemsList;
    for (auto& item : values) {
        standardItemsList.append(new QStandardItem(item));
    }
    m_csv_model->appendRow(standardItemsList);
  }
  ui->points_table->resizeRowsToContents();
  ui->points_table->resizeColumnsToContents();
}

std::vector<double> import_points_dialog_t::parse_correct_points()
{
  std::vector<double> correct_points;

  QString correct_points_str = ui->correct_points_edit->text();
  if (correct_points_str != "") {
    QStringList points_str_list = correct_points_str.split(";", QString::SkipEmptyParts);
    correct_points.reserve(points_str_list.size());
    std::transform(points_str_list.begin(), points_str_list.end(), std::back_inserter(correct_points),
      [](QString& a_str_point) {
        return a_str_point.toDouble();
      });
   }
  return correct_points;
}

void import_points_dialog_t::on_accept_points_button_clicked()
{
  if (!m_data_format_error) {
    bool rows_selected = !ui->points_table->selectionModel()->selectedRows().isEmpty();
    bool cols_selected = !ui->points_table->selectionModel()->selectedColumns().isEmpty();
    //Одновременно только или строки или столбцы
    assert(!rows_selected || !cols_selected);

    if (rows_selected || cols_selected) {
      assert(rows_selected == !cols_selected);

      emit selection_done(parse_correct_points());
      close();
    } else {
      QMessageBox::critical(this, "Error", "Select cells to continue");
    }
  } else {
    QMessageBox::critical(this, "Error", "Invalid data format");
  }
}

void import_points_dialog_t::on_cancel_button_clicked()
{
  close();
}

