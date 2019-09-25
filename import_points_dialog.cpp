#include "import_points_dialog.h"
#include "ui_import_points_form.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <algorithm>


import_points_dialog_t::import_points_dialog_t(std::vector<double> a_correct_points,
  QString a_filepath, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::Dialog),
  m_csv_filepath(a_filepath),
  m_csv_model(new QStandardItemModel(this))
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
    on_import_button_clicked();
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
    "C:\\", tr("CSV Files(*.csv)"));
  ui->filepath_edit->setText(m_csv_filepath);
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
  ui->points_table->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->points_table->setSelectionBehavior(QAbstractItemView::SelectColumns);
  ui->points_table->selectColumn(a_column);
}

void import_points_dialog_t::vhSelected(int a_row)
{
  ui->points_table->setSelectionMode(QAbstractItemView::SingleSelection);
  ui->points_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->points_table->selectRow(a_row);
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


void import_points_dialog_t::fill_x_y_arrays(std::vector<double>& a_x,
  std::vector<double>& a_y, bool a_rows_selected)
{
  if (a_rows_selected) {
    int selected_row = ui->points_table->selectionModel()->
      selectedRows().first().row();
    //Первый столбик - массив Y
    for(int i = 1; i < m_csv_model->columnCount(); i++) {
      QString x_str = m_csv_model->item(0, i)->text();
      a_x.push_back(x_str.replace(",", ".").toDouble());

      QString y_str = m_csv_model->item(selected_row, i)->text();
      a_y.push_back(y_str.replace(",", ".").toDouble());
    }
  } else {
    int selected_col = ui->points_table->selectionModel()->
      selectedColumns().first().column();
    //Первая строка - массив X
    for(int i = 1; i < m_csv_model->rowCount(); i++) {
      QString x_str = m_csv_model->item(i, 0)->text();
      a_x.push_back(x_str.replace(",", ".").toDouble());

      QString y_str = m_csv_model->item(i, selected_col)->text();
      a_y.push_back(y_str.replace(",", ".").toDouble());
    }
  }
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

    std::vector<double> x;
    std::vector<double> y;
    fill_x_y_arrays(x, y, rows_selected);

    std::vector<double>&& correct_points = parse_correct_points();
    if (correct_points.size() != 1) {

      if (!correct_points.size()) {
        correct_points.push_back(x[0]);
        correct_points.push_back(x[x.size() - 1]);
      }

      bool correct_points_are_valid = true;
      for (auto point: correct_points) {
        if (std::find(x.begin(), x.end(), point) == x.end()) {
          correct_points_are_valid = false;
          break;
        }
      }
      if (correct_points_are_valid) {
        emit points_are_ready(x, y, correct_points, m_csv_filepath);
        close();
      } else {
        QMessageBox::critical(this, "Error", "Correct points are not valid");
      }
    } else {
      QMessageBox::critical(this, "Error", "Not enough correction points");
    }
  } else {
    QMessageBox::critical(this, "Error", "Select cells to continue");
  }
}

void import_points_dialog_t::on_cancel_button_clicked()
{
  close();
}

