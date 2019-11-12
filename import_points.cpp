#include "import_points.h"

#include <QDebug>

import_points_t::import_points_t(std::vector<double> &a_correct_points, QObject *parent) :
  QObject(parent),
  m_last_file_name(""),
  m_correct_points(a_correct_points),
  m_csv_model(new QStandardItemModel(this)),
  mp_import_dialog(),
  m_selected(import_points_dialog_t::select_t::none),
  m_selected_row(0),
  m_selected_col(0)
{

}

void import_points_t::create_import_points_dialog(QWidget* a_parent)
{
  import_points_dialog_t* import_points_win = new import_points_dialog_t(
    m_correct_points, m_csv_model, m_last_file_name, a_parent);

  connect(import_points_win, &import_points_dialog_t::selection_done, this, &import_points_t::fill_data_arrays);
  connect(import_points_win, &import_points_dialog_t::filename_changed, this, &import_points_t::update_filename);
  connect(import_points_win, &import_points_dialog_t::selected_col_changed, this, &import_points_t::set_new_col);
  connect(import_points_win, &import_points_dialog_t::selected_row_changed, this, &import_points_t::set_new_row);

  import_points_win->exec();
}

void import_points_t::update_filename(const QString& a_filename)
{
  m_last_file_name = a_filename;
}

void import_points_t::set_new_row(int a_row)
{
  m_selected_row = a_row;
  m_selected = import_points_dialog_t::select_t::rows;
}

void import_points_t::set_new_col(int a_col)
{
  m_selected_col = a_col;
  m_selected = import_points_dialog_t::select_t::cols;
}

void import_points_t::fill_x_y_arrays(std::vector<double>& a_x,
  std::vector<double>& a_y)
{
  switch (m_selected) {
    case import_points_dialog_t::select_t::rows: {
      a_x.reserve(static_cast<size_t>(m_csv_model->columnCount() - 1));
      a_y.reserve(static_cast<size_t>(m_csv_model->columnCount() - 1));

      //Первый столбик - массив Y
      for(int i = 1; i < m_csv_model->columnCount(); i++) {
        QString x_str = m_csv_model->item(0, i)->text();
        a_x.push_back(x_str.replace(",", ".").toDouble());

        QString y_str = m_csv_model->item(m_selected_row, i)->text();
        a_y.push_back(y_str.replace(",", ".").toDouble());
      }
    } break;
    case import_points_dialog_t::select_t::cols: {
      a_x.reserve(static_cast<size_t>(m_csv_model->rowCount() - 1));
      a_y.reserve(static_cast<size_t>(m_csv_model->rowCount() - 1));

      //Первая строка - массив X
      for(int i = 1; i < m_csv_model->rowCount(); i++) {
        QString x_str = m_csv_model->item(i, 0)->text();
        a_x.push_back(x_str.replace(",", ".").toDouble());

        QString y_str = m_csv_model->item(i, m_selected_col)->text();
        a_y.push_back(y_str.replace(",", ".").toDouble());
      }
    } break;
    default: {
    } break;
  }

}

void import_points_t::fill_data_arrays(std::vector<double> a_correct_points)
{
  std::vector<double> x;
  std::vector<double> y;
  fill_x_y_arrays(x, y);

  bool correct_points_are_valid = true;
  for (auto point: a_correct_points) {
    if (std::find(x.begin(), x.end(), point) == x.end()) {
      correct_points_are_valid = false;
      break;
    }
  }
  m_correct_points = std::move(a_correct_points);

  if (m_correct_points.size() < 2 || !correct_points_are_valid) {
    m_correct_points = { x.front(), x.back() };
  }
  emit points_are_ready(x, y);
}

double import_points_t::get_next_data(move_direction_t a_direction)
{
  double current_x = 0;

  switch (m_selected) {
    case import_points_dialog_t::select_t::none: {
      //Точки еще не были импортированы
      return current_x;
    } break;
    case import_points_dialog_t::select_t::rows: {
      //Для вызова с a_direction == none
      QString current_x_str = m_csv_model->item(m_selected_row, 0)->text();
      current_x = current_x_str.replace(",",".").toDouble();
    } break;
    case import_points_dialog_t::select_t::cols: {
      QString current_x_str = m_csv_model->item(0, m_selected_col)->text();
      current_x = current_x_str.replace(",",".").toDouble();
    } break;
  }

  switch (a_direction) {
    case move_direction_t::up: {
      m_selected = import_points_dialog_t::select_t::rows;
      if (m_selected_row > 1) {
        m_selected_row--;
        fill_data_arrays(m_correct_points);
        current_x = m_csv_model->item(m_selected_row, 0)->text().replace(",",".").toDouble();
      }
    } break;
    case move_direction_t::down: {
      m_selected = import_points_dialog_t::select_t::rows;
      if (m_selected_row < m_csv_model->rowCount() - 1) {
        m_selected_row++;
        fill_data_arrays(m_correct_points);
        current_x = m_csv_model->item(m_selected_row, 0)->text().replace(",",".").toDouble();
      }
    } break;
    case move_direction_t::right: {
      m_selected = import_points_dialog_t::select_t::cols;
      if (m_selected_col < m_csv_model->columnCount() - 1) {
        m_selected_col++;
        fill_data_arrays(m_correct_points);
        current_x = m_csv_model->item(0, m_selected_col)->text().replace(",",".").toDouble();
      }
    } break;
    case move_direction_t::left: {
      m_selected = import_points_dialog_t::select_t::cols;
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

