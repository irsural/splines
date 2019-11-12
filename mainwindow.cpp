#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "worst_searcher.h"

#include <cmath>
#include <limits>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  m_x(),
  m_y(),
  m_correct_points { 40, 47, 70, 120, 300, 1000, 1400, 2000 },
  m_points_map(),
  m_cubic_spline(),
  m_hermite_spline(),
  m_linear_interpolation(),
  mp_linear_data(new QLineSeries(this)),
  mp_cubic_data(new QLineSeries(this)),
  mp_hermite_data(new QLineSeries(this)),
  mp_axisX(new QValueAxis(this)),
  m_min_x(0),
  m_max_x(0),
  mp_axisY(new QValueAxis(this)),
  m_min_y(std::numeric_limits<double>::max()),
  m_max_y(std::numeric_limits<double>::min()),
  m_x_step(0),
  m_auto_step(true),
  m_auto_scale(true),
  m_draw_linear(true),
  m_draw_cubic(false),
  m_draw_hermite(true),
  mp_layouts(m_x.size()),
  mp_point_checkboxes(m_x.size()),
  mp_diff_cubic_labels(m_x.size()),
  mp_diff_hermite_labels(m_x.size()),
  mp_diff_linear_labels(m_x.size()),
  m_better_color(QPalette::Window, QColor(0xbfffbd)),
  m_worst_color(QPalette::Window, QColor(0xfaa88e)),
  m_limit_color(QPalette::Window, QColor(0xffe9d1)),
  m_default_color(),
  m_draw_relative_points(false),
  m_tick_interval_count(10),
  m_start_zoom(),
  m_zoom_stack(),
  m_save_zoom(false),
  m_points_importer(new import_points_t(m_correct_points, this))
{
  ui->setupUi(this);

  create_chart();

  connect(m_points_importer, &import_points_t::points_are_ready,
    this, &MainWindow::update_points);

  m_cubic_spline.set_boundary(tk::spline::second_deriv, 0,
    tk::spline::second_deriv, 0, false);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::create_chart()
{
  mp_linear_data->setName("Linear");
  mp_cubic_data->setName("Cubic");
  mp_hermite_data->setName("Hermite");

  mp_axisX->setLabelFormat("%g");
  mp_axisY->setLabelFormat("%g");

  QChart* chart = ui->chart_widget->chart();
  chart->addAxis(mp_axisX, Qt::AlignBottom);
  chart->addAxis(mp_axisY, Qt::AlignLeft);

  ui->chart_widget->setRenderHint(QPainter::Antialiasing);
  ui->chart_widget->setRubberBand(QChartView::RectangleRubberBand);

  QHBoxLayout* header = new QHBoxLayout();
  ui->buttons_layout->addLayout(header);
  header->addWidget(new QLabel("Frequency", this));
  header->addWidget(new QLabel("Cubic", this));
  header->addWidget(new QLabel("Hermite", this));
  header->addWidget(new QLabel("Linear", this));

  ui->xmin_spinbox->setValue(m_min_x);
  ui->xmax_spinbox->setValue(m_max_x);
  ui->xstep_spinbox->setValue(m_x_step);

  connect(mp_axisX, &QValueAxis::rangeChanged, this, &MainWindow::chart_was_zoomed);
  connect(mp_axisY, &QValueAxis::rangeChanged, this, &MainWindow::chart_was_zoomed);
}

void MainWindow::create_control(const vector<double>& a_x)
{
  size_t row_ind = 0;
  for(auto x: a_x) {
    mp_layouts[row_ind] = new QHBoxLayout();
    ui->buttons_layout->addLayout(mp_layouts[row_ind]);

    mp_point_checkboxes[row_ind] = new QCheckBox(QString::number(x), this);
    mp_layouts[row_ind]->addWidget(mp_point_checkboxes[row_ind]);

    if (std::find(m_correct_points.begin(), m_correct_points.end(), x)
      != m_correct_points.end())
    {
      mp_point_checkboxes[row_ind]->setChecked(true);
    }
    connect(mp_point_checkboxes[row_ind], &QCheckBox::clicked, this,
      &MainWindow::redraw_spline);

    mp_diff_cubic_labels[row_ind] = new QLabel("0.00000", this);
    mp_diff_cubic_labels[row_ind]->setAutoFillBackground(true);
    mp_layouts[row_ind]->addWidget(mp_diff_cubic_labels[row_ind]);

    mp_diff_hermite_labels[row_ind] = new QLabel("0.00000", this);
    mp_diff_hermite_labels[row_ind]->setAutoFillBackground(true);
    mp_layouts[row_ind]->addWidget(mp_diff_hermite_labels[row_ind]);

    mp_diff_linear_labels[row_ind] = new QLabel("0.00000", this);
    mp_diff_linear_labels[row_ind]->setAutoFillBackground(true);
    mp_layouts[row_ind]->addWidget(mp_diff_linear_labels[row_ind]);

    row_ind++;
  }
}


void MainWindow::calc_splines(const vector<double> &a_correct_points)
{
  assert(m_x.size() == m_y.size());
  m_points_map.clear();

  for (size_t i = 0; i < m_x.size(); i++) {
    m_points_map[m_x[i]] = m_y[i];
  }

  vector<double> correct_values;
  for (auto point: a_correct_points) {
    auto value = m_points_map.find(point);
    if (value != m_points_map.end()) {
      correct_values.push_back(value->second);
    }
  }

  m_cubic_spline.set_points(a_correct_points, correct_values);
  m_hermite_spline.set_points(a_correct_points.data(), correct_values.data(), a_correct_points.size());
  m_linear_interpolation.set_points(a_correct_points.data(), correct_values.data(), a_correct_points.size());

  calc_difs();
}

double MainWindow::get_diff(double a_real, double a_calculated)
{
  return (a_real - a_calculated) / a_calculated * 100;
}


void MainWindow::calc_difs()
{
  worst_searcher_t<double> worst_cubic;
  worst_searcher_t<double> worst_hermite;
  worst_searcher_t<double> worst_linear;

  size_t label_ind = 0;
  for (auto a_pair: m_points_map) {
    double real_value = a_pair.second;

    double cubic_value = m_cubic_spline(a_pair.first);
    double cubic_diff = get_diff(real_value, cubic_value);
    mp_diff_cubic_labels[label_ind]->setText(QString::number(cubic_diff));
    mp_diff_cubic_labels[label_ind]->setPalette(m_default_color);

    double hermite_value = m_hermite_spline(a_pair.first);
    double hermite_diff = get_diff(real_value, hermite_value);
    mp_diff_hermite_labels[label_ind]->setText(QString::number(hermite_diff));
    mp_diff_hermite_labels[label_ind]->setPalette(m_default_color);

    double linear_value = m_linear_interpolation(a_pair.first);
    double linear_diff = get_diff(real_value, linear_value);
    mp_diff_linear_labels[label_ind]->setText(QString::number(linear_diff));
    mp_diff_linear_labels[label_ind]->setPalette(m_default_color);


    if (abs(cubic_diff) < abs(hermite_diff)) {
      mp_diff_cubic_labels[label_ind]->setPalette(m_better_color);
    } else if (abs(cubic_diff) > abs(hermite_diff)) {
      mp_diff_hermite_labels[label_ind]->setPalette(m_better_color);
    }

    if (abs(cubic_diff) > ui->spinbox_mark_limit->value()) {
      mp_diff_cubic_labels[label_ind]->setPalette(m_limit_color);
    }
    if (abs(hermite_diff) > ui->spinbox_mark_limit->value()) {
      mp_diff_hermite_labels[label_ind]->setPalette(m_limit_color);
    }
    if (abs(linear_diff) > ui->spinbox_mark_limit->value()) {
      mp_diff_linear_labels[label_ind]->setPalette(m_limit_color);
    }

    worst_cubic.add(abs(cubic_diff));
    worst_hermite.add(abs(hermite_diff));
    worst_linear.add(abs(linear_diff));

    label_ind++;
  }
  mp_diff_cubic_labels[worst_cubic.get_index()]->setPalette(m_worst_color);
  mp_diff_hermite_labels[worst_hermite.get_index()]->setPalette(m_worst_color);
  mp_diff_linear_labels[worst_linear.get_index()]->setPalette(m_worst_color);
}

void MainWindow::draw_lines(double a_min, double a_max, double a_step)
{
  mp_cubic_data->clear();
  mp_hermite_data->clear();
  mp_linear_data->clear();

  mp_axisX->setTickType(QValueAxis::TickType::TicksFixed);
  mp_axisY->setTickType(QValueAxis::TickType::TicksFixed);

  m_min_y = std::numeric_limits<double>::max();
  m_max_y = std::numeric_limits<double>::min();

  double relative_point = m_points_map[m_correct_points[0]] / m_correct_points[0];

  for (double i = a_min; i < a_max; i += a_step) {
    double cubic_val = m_cubic_spline(i);
    double hermite_val = m_hermite_spline(i);

    if (m_draw_relative_points) {
      cubic_val = (cubic_val/i - relative_point) * 100;
      hermite_val = (hermite_val/i - relative_point) * 100;
    }
    if (m_draw_cubic) {
      m_min_y = m_min_y > cubic_val ? cubic_val : m_min_y;
      m_max_y = m_max_y < cubic_val ? cubic_val : m_max_y;
      mp_cubic_data->append(i, cubic_val);
    }
    if (m_draw_hermite) {
      m_min_y = m_min_y > hermite_val ? hermite_val : m_min_y;
      m_max_y = m_max_y < hermite_val ? hermite_val : m_max_y;
      mp_hermite_data->append(i, hermite_val);
    }
  }

  for (size_t i = 0; i < m_x.size(); i++) {
    double value = m_y[i];
    if (m_draw_relative_points) {
      value = (value/m_x[i] - relative_point) * 100;
    }
    if (m_draw_linear) {
      m_min_y = m_min_y > value ? value : m_min_y;
      m_max_y = m_max_y < value ? value : m_max_y;
      mp_linear_data->append(m_x[i], value);
    }
  }

  static bool drawed = false;
  if (!drawed) {
    ui->chart_widget->chart()->addSeries(mp_cubic_data);
    ui->chart_widget->chart()->addSeries(mp_hermite_data);
    ui->chart_widget->chart()->addSeries(mp_linear_data);

    mp_cubic_data->attachAxis(mp_axisX);
    mp_cubic_data->attachAxis(mp_axisY);
    mp_hermite_data->attachAxis(mp_axisX);
    mp_hermite_data->attachAxis(mp_axisY);
    mp_linear_data->attachAxis(mp_axisX);
    mp_linear_data->attachAxis(mp_axisY);
    drawed = true;
  }
  if (m_auto_scale) {
    mp_axisX->setRange(a_min, a_max);
    mp_axisY->setRange(m_min_y, m_max_y);
    set_new_zoom_start();
  }

  double x_tick_interval = calc_chart_tick_interval(mp_axisX->min(), mp_axisX->max(), m_tick_interval_count);
  mp_axisX->setTickInterval(x_tick_interval);
  mp_axisX->setTickType(QValueAxis::TickType::TicksDynamic);

  double y_tick_interval = calc_chart_tick_interval(mp_axisY->min(), mp_axisY->max(), m_tick_interval_count);
  mp_axisY->setTickInterval(y_tick_interval);
  mp_axisY->setTickType(QValueAxis::TickType::TicksDynamic);
}

std::tuple<double, double> get_double_power(double a_val)
{
  //���������� �������� � ������ base(10) ���������� ��� double �����
  double value = 0;
  double power = 0;
  if (a_val > std::numeric_limits<double>::epsilon()) {
    power = int(std::floor(std::log10(std::fabs(a_val))));
    value = a_val * std::pow(10 , -1*power);
  }
  return std::make_tuple(value, power);
}

double MainWindow::calc_chart_tick_interval(double a_min, double a_max,
  size_t a_ticks_count)
{
  std::array<double, 3> nice_numbers{ 1, 2, 5 };
  double bad_tick_interval = (a_max - a_min) / a_ticks_count;

  auto [val, power] = get_double_power(bad_tick_interval);

  double nice = 1;
  double diff = numeric_limits<double>::max();

  for (auto number: nice_numbers) {
    double cur_diff = std::abs(number - val);
    if (diff > cur_diff) {
      diff = cur_diff;
      nice = number;
    }
  }
  return nice * pow(10, power);
}


void MainWindow::fill_cubic(vector<double> &a_x, vector<double> &a_y,
  double a_min, double a_max, double a_step)
{
  a_x.clear();
  a_y.clear();
  size_t points_count = static_cast<size_t>((a_max - a_min) / a_step);
  a_x.reserve(points_count);
  a_y.reserve(points_count);

  double relative_point = m_points_map[m_correct_points[0]] / m_correct_points[0];

  for (double i = a_min; i < a_max; i += a_step) {
    double cubic_val = m_cubic_spline(i);

    if (m_draw_relative_points) {
      cubic_val = (cubic_val/i - relative_point) * 100;
    }
    a_x.push_back(i);
    a_y.push_back(cubic_val);
  }
}

void MainWindow::fill_hermite(vector<double>& a_x, vector<double>& a_y,
  double a_min, double a_max, double a_step)
{
  a_x.clear();
  a_y.clear();
  size_t points_count = static_cast<size_t>((a_max - a_min) / a_step);
  a_x.reserve(points_count);
  a_y.reserve(points_count);

  double relative_point = m_points_map[m_correct_points[0]] / m_correct_points[0];

  for (double i = a_min; i < a_max; i += a_step) {
    double hermite_val = m_hermite_spline(i);

    if (m_draw_relative_points) {
      hermite_val = (hermite_val/i - relative_point) * 100;
    }
    a_x.push_back(i);
    a_y.push_back(hermite_val);
  }
}

void MainWindow::fill_linear(vector<double>& a_x, vector<double>& a_y)
{
  a_x.clear();
  a_y.clear();
  a_x.reserve(m_x.size());
  a_y.reserve(m_x.size());

  double relative_point = m_points_map[m_correct_points[0]] / m_correct_points[0];

  for (size_t i = 0; i < m_x.size(); i++) {
    double value = m_y[i];
    if (m_draw_relative_points) {
      value = (value/m_x[i] - relative_point) * 100;
    }
    a_x.push_back(m_x[i]);
    a_y.push_back(value);
  }
}

void MainWindow::draw_line(const vector<double>& a_x, const vector<double> &a_y, QLineSeries* a_series)
{
  a_series->clear();
  double min_y = std::numeric_limits<double>::max();
  double max_y = std::numeric_limits<double>::min();

  for (size_t i = 0; i < a_x.size(); i++) {
    double val = a_y[i];
    a_series->append(a_x[i], val);

    min_y = min_y > val ? val : min_y;
    max_y = max_y < val ? val : max_y;
  }
  m_max_y = m_max_y < max_y ? max_y : m_max_y;
  m_min_y = m_min_y > min_y ? min_y : m_min_y;

  if (m_auto_scale) {
    mp_axisY->setRange(m_min_y, m_max_y);
    mp_axisX->setRange(a_x[0], a_x[a_x.size() - 1]);
  }

  ui->chart_widget->chart()->addSeries(a_series);

  a_series->attachAxis(mp_axisX);
  a_series->attachAxis(mp_axisY);
}


void MainWindow::redraw_spline(bool a_checked)
{
  QObject* obj = QObject::sender();
  QCheckBox* pressed_cb = qobject_cast<QCheckBox*>(obj);
  double point_val = double(pressed_cb->text().toDouble());

  if (a_checked) {
    m_correct_points.push_back(point_val);
    std::sort(m_correct_points.begin(), m_correct_points.end());
  } else {
    if (m_correct_points.size() > 2) {
      m_correct_points.erase(std::remove(m_correct_points.begin(),
        m_correct_points.end(), point_val), m_correct_points.end());
    } else {
      pressed_cb->setChecked(true);
    }
  }
  repaint_spline();
}

void MainWindow::reinit_control_buttons()
{
  for (size_t i = 0; i < mp_layouts.size(); i++) {
    if (mp_layouts[i] != nullptr) delete mp_layouts[i];
    if (mp_point_checkboxes[i] != nullptr) delete mp_point_checkboxes[i];
    if (mp_diff_cubic_labels[i] != nullptr) delete mp_diff_cubic_labels[i];
    if (mp_diff_hermite_labels[i] != nullptr) delete mp_diff_hermite_labels[i];
    if (mp_diff_linear_labels[i] != nullptr) delete mp_diff_linear_labels[i];
  }

  m_min_x = m_x.front();
  m_max_x = m_x.back();
  m_min_y = std::numeric_limits<double>::max();
  m_max_y = std::numeric_limits<double>::min();
  m_x_step = m_auto_step ? (m_x.back() - m_x.front()) / 400 : m_x_step;
  mp_layouts.resize(m_x.size());
  mp_point_checkboxes.resize(m_x.size());
  mp_diff_cubic_labels.resize(m_x.size());
  mp_diff_hermite_labels.resize(m_x.size());
  mp_diff_linear_labels.resize(m_x.size());

  ui->xmin_spinbox->setValue(m_min_x);
  ui->xmax_spinbox->setValue(m_max_x);
  ui->xstep_spinbox->setValue(m_x_step);

  create_control(m_x);
}

MainWindow::input_data_error_t MainWindow::verify_data(const vector<double>& a_x, const vector<double>& a_y)
{
  if (a_x.empty() || a_y.empty()) {
    return input_data_error_t::no_data;
  }
  if (a_x.size() != a_y.size()) {
    return input_data_error_t::arrays_not_same_size;
  }
  if (a_x.size() < 2) {
    return input_data_error_t::not_enough_points;
  }
  for(size_t i = 1; i < a_x.size(); i++) {
    if (a_x[i] <= a_x[i-1]) {
      return input_data_error_t::not_increasing_sequence;
    }
  }
  return input_data_error_t::none;
}

void MainWindow::update_points(vector<double> &a_x, vector<double> &a_y)
{
  input_data_error_t error = verify_data(a_x, a_y);

  switch(error) {
    case input_data_error_t::none: {
      m_x = std::move(a_x);
      m_y = std::move(a_y);

      reinit_control_buttons();
      repaint_spline();

      ui->current_x_label->setText(m_points_importer->get_x());
    } break;
    case input_data_error_t::no_data: {
      QMessageBox::critical(this, "Error", "X or Y array is empty");
    } break;
    case input_data_error_t::arrays_not_same_size: {
      QMessageBox::critical(this, "Error", "X and Y arrays must be same size");
    } break;
    case input_data_error_t::not_enough_points: {
      QMessageBox::critical(this, "Error", "Not enough points to calculate spline");
    } break;
    case input_data_error_t::not_increasing_sequence: {
      QMessageBox::critical(this, "Error", "X array must be strictly increasing sequence ");
    } break;
  }
}

void MainWindow::repaint_spline()
{
  if (!m_x.empty()) {
    m_save_zoom = false;
    calc_splines(m_correct_points);
    draw_lines(m_min_x, m_max_x, m_x_step);
    m_save_zoom = true;
  }
}

void MainWindow::on_xmin_spinbox_valueChanged(double a_val)
{
  m_min_x = a_val;
  mp_axisX->setMin(m_min_x);
}

void MainWindow::on_xmax_spinbox_valueChanged(double a_val)
{
  m_max_x = a_val;
  mp_axisX->setMax(m_max_x);
}

void MainWindow::on_reset_scale_clicked()
{
  //����� ����� �������� ������
  mp_axisX->setTickInterval(m_max_x - m_min_x);
  mp_axisY->setTickInterval(m_max_y - m_min_y);

  mp_axisX->setRange(m_min_x, m_max_x);
  mp_axisY->setRange(m_min_y, m_max_y);
  repaint_spline();
  set_new_zoom_start();
}

void MainWindow::on_open_file_button_clicked()
{
  m_points_importer->create_import_points_dialog(this);
}

void MainWindow::on_xstep_spinbox_valueChanged(double a_val)
{
  m_x_step = a_val;
}

void MainWindow::on_checkBox_stateChanged(int a_state)
{
  m_draw_relative_points = a_state;
  repaint_spline();
}

void MainWindow::on_checkBox_2_stateChanged(int a_state)
{
  m_auto_step = a_state;
  repaint_spline();
}

void MainWindow::on_checkBox_3_stateChanged(int a_state)
{
  m_auto_scale = a_state;
}

void MainWindow::on_checkBox_4_stateChanged(int a_state)
{
  m_draw_linear = a_state;
  repaint_spline();
}

void MainWindow::on_checkBox_5_stateChanged(int a_state)
{
  m_draw_cubic = a_state;
  repaint_spline();
}

void MainWindow::on_checkBox_6_stateChanged(int a_state)
{
  m_draw_hermite = a_state;
  repaint_spline();
}

void MainWindow::keyPressEvent(QKeyEvent *a_event)
{
  if (a_event->nativeScanCode() == 17/*w*/) {
    m_points_importer->set_next_data(import_points_t::move_direction_t::up);
  } else if (a_event->nativeScanCode() == 31/*s*/) {
    m_points_importer->set_next_data(import_points_t::move_direction_t::down);
  } else if (a_event->nativeScanCode() == 30/*a*/) {
    m_points_importer->set_next_data(import_points_t::move_direction_t::left);
  } else if (a_event->nativeScanCode() == 32/*d*/) {
    m_points_importer->set_next_data(import_points_t::move_direction_t::right);
  }
}

void MainWindow::on_show_all_graps_button_clicked()
{
}

void MainWindow::on_button_apply_clicked()
{
  bool prev_auto_scale = m_auto_scale;
  m_auto_scale = false;
  repaint_spline();
  m_auto_scale = prev_auto_scale;
}

void MainWindow::chart_was_zoomed(qreal a_min, qreal a_max)
{
  QObject* obj = QObject::sender();
  QValueAxis* zoomed_axis = qobject_cast<QValueAxis*>(obj);

  double tick_interval = calc_chart_tick_interval(a_min, a_max, m_tick_interval_count);
  zoomed_axis->setTickInterval(tick_interval);
  zoomed_axis->setTickType(QValueAxis::TickType::TicksDynamic);

  if (zoomed_axis == mp_axisY) {
    if (m_save_zoom) {
      //������� �� Y ������ ���������� ����� �������� �� X
      m_zoom_stack.push(QRectF(mp_axisX->min(), mp_axisX->max(),
        mp_axisY->min(), mp_axisY->max()));
    }
  }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *a_event)
{
  if (ui->chart_layout->geometry().intersects(
    QRect(a_event->pos(), QSize(1, 1))))
  {
    if (!m_zoom_stack.empty()) {
      if (m_zoom_stack.top() != m_start_zoom) {
        m_zoom_stack.pop();

        const QRectF& field = m_zoom_stack.top();

        bool prev_auto_scale = m_auto_scale;
        m_auto_scale = false;
        m_save_zoom = false;
        mp_axisX->setRange(field.left(), field.top());
        mp_axisY->setRange(field.width(), field.height());
        repaint_spline();
        m_save_zoom = true;
        m_auto_scale = prev_auto_scale;
      }
    }
  }
}

void MainWindow::set_new_zoom_start()
{
  m_start_zoom = {m_min_x, m_max_x, m_min_y, m_max_y};
  while(!m_zoom_stack.empty()) m_zoom_stack.pop();
  m_zoom_stack.push(m_start_zoom);
  m_save_zoom = true;
}
