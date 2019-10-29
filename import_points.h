#ifndef IMPORT_POINTS_H
#define IMPORT_POINTS_H

#include <QObject>
#include <QWidget>
#include "import_points_dialog.h"

class import_points_t : public QObject
{
  Q_OBJECT
public:
  enum class move_direction_t { up, down, left, right, none };

  explicit import_points_t(QObject *parent = nullptr);

  void create_import_points_dialog(QWidget *a_parent);
  void set_correct_points(const std::vector<double>& a_correct_points);
  double get_next_data(move_direction_t a_direction);

  int get_rows_count();
  int get_cols_count();
  bool is_rows_selected();
  void get_data(std::vector<double> a_x, std::vector<double> a_y,
    int a_data_num);

signals:
  void points_are_ready(std::vector<double> a_x, std::vector<double> a_y,
    std::vector<double> a_correct_points);

private slots:
  void update_filename(QString a_filename);
  void set_new_row(int a_row);
  void set_new_col(int a_col);

  void fill_data_arrays(std::vector<double> a_correct_points);

private:
  QString m_last_file_name;
  std::vector<double> m_correct_points;
  QStandardItemModel *m_csv_model;

  int m_selected_row;
  int m_selected_col;
  bool m_rows_selected;

  import_points_dialog_t* mp_import_dialog;

  void fill_x_y_arrays(std::vector<double> &a_x,
    std::vector<double> &a_y, int a_row, int a_col, bool a_rows_selected);
};

#endif // IMPORT_POINTS_H
