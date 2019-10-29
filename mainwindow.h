#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QCategoryAxis>
#include <QDateTimeAxis>
#include <QDebug>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QHBoxLayout>

#include <limits>
#include <cmath>

#include "spline.h"
#include "hermit.h"
#include "import_points.h"
#include "linear_interpolation.hpp"


using namespace std;

QT_CHARTS_USE_NAMESPACE


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void redraw_spline(bool a_checked);
  void on_xmin_spinbox_valueChanged(double a_val);
  void on_xmax_spinbox_valueChanged(double a_val);
  void on_reset_scale_clicked();
  void on_open_file_button_clicked();
  void on_xstep_spinbox_valueChanged(double arg1);
  void on_checkBox_stateChanged(int a_state);
  void on_checkBox_2_stateChanged(int a_state);
  void on_checkBox_3_stateChanged(int a_state);
  void on_checkBox_4_stateChanged(int a_state);
  void on_checkBox_5_stateChanged(int a_state);
  void on_checkBox_6_stateChanged(int a_state);
  void update_points(vector<double> a_x, vector<double> a_y,
    vector<double> a_correct_points);
  void on_show_all_graps_button_clicked();
  void on_button_apply_ticks_count_clicked();

  void chart_was_zoomed(qreal min, qreal max);

private:
  Ui::MainWindow *ui;

  vector<double> m_freq;
  vector<double> m_current;
  vector<double> m_correct_points;
  std::map<double,double> m_points;

  QChart* mp_chart;
  QChartView* mp_chart_view;

  ::tk::spline m_cubic_spline;
  pchip_t<double> m_hermite_spline;
  irs::line_interp_t<double> m_linear_interpolation;

  QLineSeries* mp_linear_data;
  QLineSeries* mp_cubic_data;
  QLineSeries* mp_hermite_data;

  QValueAxis *mp_axisX;
  double m_min_x;
  double m_max_x;
  QValueAxis *mp_axisY;
  double m_min_y;
  double m_max_y;

  double m_x_step;
  bool m_auto_step;
  bool m_auto_scale;

  bool m_draw_linear;
  bool m_draw_cubic;
  bool m_draw_hermite;

  vector<QHBoxLayout*> mp_layouts;
  vector<QCheckBox*> mp_point_checkboxes;
  vector<QLabel*> mp_diff_cubic_labels;
  vector<QLabel*> mp_diff_hermite_labels;
  vector<QLabel*> mp_diff_linear_labels;

  QPalette m_better_color;
  QPalette m_worst_color;
  QPalette m_limit_color;
  QPalette m_default_color;

  bool m_draw_relative_points;

  import_points_t* m_points_importer;

  void create_chart();
  void create_control(const vector<double>& a_x);
  void calc_splines(vector<double> &a_points);
  void calc_difs();
  void draw_lines(double a_min, double a_max, double a_step);
  double calc_chart_tick_interval(double a_min, double a_max,
    size_t a_ticks_count);

  void fill_cubic(vector<double>& a_x, vector<double>& a_y, double a_min,
    double a_max, double a_step);
  void fill_hermite(vector<double>& a_x, vector<double>& a_y,
    double a_min, double a_max, double a_step);
  void fill_linear(vector<double>& a_x, vector<double>& a_y);
  void draw_line(const vector<double>& a_x, const vector<double> &a_y,
    QLineSeries* a_series);

  void repaint_spline();
  void reinit_control_buttons();

  void keyPressEvent(QKeyEvent *a_event);
};

#endif // MAINWINDOW_H
