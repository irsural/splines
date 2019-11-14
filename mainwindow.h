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
#include <memory>
#include <stack>

#include "spline.h"
#include "hermit.h"
#include "import_points.h"
#include "linear_interpolation.hpp"
#include "peak_searcher.h"

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
  void update_points(vector<double> &a_x, vector<double> &a_y);
  void on_button_apply_clicked();
  void chart_was_zoomed(qreal min, qreal max);
  void on_spinbox_mark_limit_valueChanged(double arg1);
  void on_draw_linear_checkbox_stateChanged(int arg1);
  void on_draw_cubic_checkbox_stateChanged(int arg1);
  void on_draw_hermite_checkbox_stateChanged(int arg1);

private:
  enum class input_data_error_t {
    none,
    no_data,
    not_enough_points,
    not_increasing_sequence,
    arrays_not_same_size
  };

  enum interpolation_type_t {
    it_cubic = 0,
    it_hermite = 1,
    it_linear = 2,
    it_count = 3
  };

  struct interpolation_t {
    interpolation_base_t &interpolation;
    QLineSeries* series;
    vector<QLabel*> deviation_labels;
    peak_searcher_t<double> worst_point;
    bool enable;

    interpolation_t(interpolation_base_t &a_interpolation, QLineSeries *a_series):
      interpolation(a_interpolation),
      series(a_series),
      deviation_labels(),
      worst_point(),
      enable(false)
    {
    }
  };

  Ui::MainWindow *ui;

  vector<double> m_x;
  vector<double> m_y;
  vector<double> m_correct_points;
  std::map<double,double> m_points_map;

  ::tk::spline m_cubic_spline;
  pchip_t<double> m_hermite_spline;
  irs::line_interp_t<double> m_linear_interpolation;
  vector<std::unique_ptr<interpolation_t>> m_interpolation_data;

  QLineSeries *m_data_series;

  QValueAxis *mp_axisX;
  double m_min_x;
  double m_max_x;
  QValueAxis *mp_axisY;
  double m_min_y;
  double m_max_y;

  double m_x_step;
  bool m_auto_step;
  bool m_auto_scale;

  vector<QHBoxLayout*> mp_deviation_layouts;
  vector<QCheckBox*> mp_point_checkboxes;

  QPalette m_best_color;
  QPalette m_worst_color;
  QPalette m_limit_color;
  QPalette m_default_color;

  double m_mark_limit;
  bool m_draw_relative_points;
  size_t m_tick_interval_count;

  QRectF m_start_zoom;
  std::stack<QRectF> m_zoom_stack;
  bool m_save_zoom;

  import_points_t* m_points_importer;

  void create_chart();
  void create_control(const vector<double>& a_x);
  input_data_error_t verify_data(const vector<double>& a_x, const vector<double>& a_y);
  void calc_splines(const vector<double> &a_correct_points);
  double deviation(double a_real, double a_calculated);
  void calc_deviations();
  void set_nice_axis_numbers(QValueAxis *a_axis, double a_min, double a_max, size_t a_ticks_count);
  void draw_lines(double a_min, double a_max, double a_step);
  double calc_chart_tick_interval(double a_min, double a_max, size_t a_ticks_count);

  void repaint_data_line();
  void repaint_spline();

  void reset_deviation_layouts();
  void reinit_control_buttons();

  void keyPressEvent(QKeyEvent *a_event);
  void mouseDoubleClickEvent(QMouseEvent *event);
  void set_new_zoom_start();
};

#endif // MAINWINDOW_H
