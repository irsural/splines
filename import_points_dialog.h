#ifndef IMPORT_POINTS_DIALOG_H
#define IMPORT_POINTS_DIALOG_H

#include <QDialog>
#include <QFile>
#include <QStandardItemModel>
#include <QAbstractButton>

namespace Ui {
class Dialog;
}

class import_points_dialog_t : public QDialog
{
  Q_OBJECT

public:
  explicit import_points_dialog_t(std::vector<double> a_correct_points,
    QString a_filepath = "", QWidget *parent = nullptr);
  ~import_points_dialog_t();

signals:
  void points_are_ready(std::vector<double> a_x, std::vector<double> a_y,
    std::vector<double> a_correct_points, QString a_filename);


private slots:
  void on_choose_file_button_clicked();
  void on_import_button_clicked();
  void hhSelected(int a_column);
  void vhSelected(int a_row);
  void on_cancel_button_clicked();
  void on_accept_points_button_clicked();

private:
  Ui::Dialog *ui;

  QString m_csv_filepath;
  QStandardItemModel *m_csv_model;

  void insert_points_to_table(QFile* a_file);
  void resize_dialog_for_table();
  std::vector<double> parse_correct_points();
  void fill_x_y_arrays(std::vector<double> &a_x,
    std::vector<double> &a_y, bool a_rows_selected);
};

#endif // IMPORT_POINTS_DIALOG_H
