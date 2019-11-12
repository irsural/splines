#ifndef IMPORT_POINTS_DIALOG_H
#define IMPORT_POINTS_DIALOG_H

#include <QDialog>
#include <QFile>
#include <QStandardItemModel>
#include <QAbstractButton>
#include <QStandardItemModel>

namespace Ui {
class Dialog;
}

class import_points_dialog_t : public QDialog
{
  Q_OBJECT

public:
  enum class select_t { none, rows, cols };

  explicit import_points_dialog_t(const std::vector<double>& a_correct_points,
    QStandardItemModel *a_csv_model, QString a_filepath = "",
    QWidget *parent = nullptr);
  ~import_points_dialog_t();

signals:
  void selection_done(std::vector<double> a_correct_points);
  void filename_changed(QString a_filename);

  void selected_row_changed(int a_row);
  void selected_col_changed(int a_col);

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
  bool m_data_format_error;

  void insert_points_to_table(QFile* a_file);
  std::vector<double> parse_correct_points();
};

#endif // IMPORT_POINTS_DIALOG_H
