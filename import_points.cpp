#include "import_points.h"

#include <QDebug>

import_points_t::import_points_t(QObject *parent) :
  QObject(parent),
  m_last_file_name(""),
  m_correct_points(),
  m_csv_model(new QStandardItemModel(this)),
  m_selected_row(0),
  m_selected_col(0),
  m_rows_selected(false),
  mp_import_dialog()
{

}

void import_points_t::create_import_points_dialog(QWidget* a_parent)
{
  import_points_dialog_t* import_points_win = new import_points_dialog_t(
    m_correct_points, m_csv_model, m_last_file_name, a_parent);

  connect(import_points_win, &import_points_dialog_t::selection_done, this,
    &import_points_t::fill_data_arrays);
  connect(import_points_win, &import_points_dialog_t::filename_changed, this,
    &import_points_t::update_filename);
  connect(import_points_win, &import_points_dialog_t::selected_col_changed, this,
    &import_points_t::set_new_col);
  connect(import_points_win, &import_points_dialog_t::selected_row_changed, this,
    &import_points_t::set_new_row);

  import_points_win->exec();
}

void import_points_t::set_correct_points(const std::vector<double>& a_correct_points)
{
  m_correct_points = a_correct_points;
}

void import_points_t::update_filename(QString a_filename)
{
  m_last_file_name = a_filename;
}

void import_points_t::set_new_row(int a_row)
{
  m_selected_row = a_row;
  m_rows_selected = true;
}

void import_points_t::set_new_col(int a_col)
{
  m_selected_col = a_col;
  m_rows_selected = false;
}

void import_points_t::fill_x_y_arrays(std::vector<double>& a_x,
  std::vector<double>& a_y, bool a_rows_selected)
{
  if (a_rows_selected) {
    //Первый столбик - массив Y
    for(int i = 1; i < m_csv_model->columnCount(); i++) {
      QString x_str = m_csv_model->item(0, i)->text();
      a_x.push_back(x_str.replace(",", ".").toDouble());

      QString y_str = m_csv_model->item(m_selected_row, i)->text();
      a_y.push_back(y_str.replace(",", ".").toDouble());
    }
  } else {
    //Первая строка - массив X
    for(int i = 1; i < m_csv_model->rowCount(); i++) {
      QString x_str = m_csv_model->item(i, 0)->text();
      a_x.push_back(x_str.replace(",", ".").toDouble());

      QString y_str = m_csv_model->item(i, m_selected_col)->text();
      a_y.push_back(y_str.replace(",", ".").toDouble());
    }
  }
}

void import_points_t::fill_data_arrays(std::vector<double> a_correct_points)
{
  std::vector<double> x;
  std::vector<double> y;
  fill_x_y_arrays(x, y, m_rows_selected);

  bool correct_points_are_valid = true;
  for (auto point: a_correct_points) {
    if (std::find(x.begin(), x.end(), point) == x.end()) {
      correct_points_are_valid = false;
      break;
    }
  }

  if (a_correct_points.size() < 2 || !correct_points_are_valid) {
    a_correct_points.clear();
    a_correct_points.push_back(x[0]);
    a_correct_points.push_back(x[x.size() - 1]);
  }
  m_correct_points = a_correct_points;

  emit points_are_ready(x, y, m_correct_points);
}

double import_points_t::get_next_data(move_direction_t a_direction)
{
  if (!m_selected_col && !m_selected_row) {
    return 0;
  }

  QString current_x_str = m_rows_selected ? m_csv_model->item(
    m_selected_row, 0)->text() : m_csv_model->item(0, m_selected_col)->text();
  double current_x = current_x_str.replace(",",".").toDouble();

  switch (a_direction) {
    case move_direction_t::up: {
      m_rows_selected = true;
      if (m_selected_row > 1) {
        m_selected_row--;
        fill_data_arrays(m_correct_points);
        current_x = m_csv_model->item(m_selected_row, 0)->text().replace(",",".").toDouble();
      }
    } break;

    case move_direction_t::down: {
      m_rows_selected = true;
      if (m_selected_row < m_csv_model->rowCount() - 1) {
        m_selected_row++;
        fill_data_arrays(m_correct_points);
        current_x = m_csv_model->item(m_selected_row, 0)->text().replace(",",".").toDouble();
      }
    } break;
    case move_direction_t::right: {
      m_rows_selected = false;
      if (m_selected_col < m_csv_model->columnCount() - 1) {
        m_selected_col++;
        fill_data_arrays(m_correct_points);
        current_x = m_csv_model->item(0, m_selected_col)->text().replace(",",".").toDouble();
      }
    } break;
    case move_direction_t::left: {
      m_rows_selected = false;
      if (m_selected_col > 1) {
        m_selected_col--;
        fill_data_arrays(m_correct_points);
        current_x = m_csv_model->item(0, m_selected_col)->text().replace(",",".").toDouble();
      }
    } break;
    default: break;
  }
  return current_x;
}
