#ifndef IMPORT_POINTS_H
#define IMPORT_POINTS_H

#include <QObject>
#include <QWidget>
#include "import_points_dialog.h"

class import_points_t : public QObject
{
  Q_OBJECT
public:
  enum class move_direction_t { up, down, left, right };

  explicit import_points_t(std::vector<double>& a_correct_points, QObject *parent = nullptr);

  void create_import_points_dialog(QWidget *a_parent);
  bool are_correct_points_valid(const std::vector<double> &a_correct_points, const std::vector<double> &a_x);
  void set_next_data(move_direction_t a_direction);
  import_points_dialog_t::select_t get_select_type();
  QString get_x();

signals:
  void points_are_ready(std::vector<double> &a_x, std::vector<double> &a_y);

private slots:
  void update_filename(const QString &a_filename);
  void set_new_row(int a_row);
  void set_new_col(int a_col);
  void fill_data_arrays(std::vector<double> a_correct_points);

private:
  QString m_last_file_name;
  std::vector<double> &m_correct_points;
  QStandardItemModel *m_csv_model;

  import_points_dialog_t* mp_import_dialog;

  import_points_dialog_t::select_t m_selected;
  int m_selected_row;
  int m_selected_col;

  void fill_x_y_arrays(std::vector<double> &a_x, std::vector<double> &a_y);
};

#endif // IMPORT_POINTS_H
