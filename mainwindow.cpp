#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <cmath>
#include <limits>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <chrono>

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
  m_interpolation_data(),
  m_data_series(new QLineSeries(this)),
  mp_axisX(new QValueAxis(this)),
  m_min_x(0),
  m_max_x(0),
  mp_axisY(new QValueAxis(this)),
  m_min_y(std::numeric_limits<double>::max()),
  m_max_y(std::numeric_limits<double>::min()),
  m_x_step(0),
  m_auto_step(true),
  m_auto_scale(true),
  mp_deviation_layouts(m_x.size()),
  mp_point_checkboxes(m_x.size()),
  m_best_color(QPalette::Window, QColor(0xbfffbd)),
  m_worst_color(QPalette::Window, QColor(0xfaa88e)),
  m_limit_color(QPalette::Window, QColor(0xffe9d1)),
  m_default_color(),
  m_mark_limit(0.01),
  m_draw_relative_points(false),
  m_tick_interval_count(10),
  m_start_zoom(),
  m_zoom_stack(),
  m_save_zoom(false),
  m_points_importer(new import_points_t(m_correct_points, this))
{
  ui->setupUi(this);

  m_interpolation_data.reserve(it_count);
  //Порядок интерполяций должен соответствовать enum interpolation_type_t
  m_interpolation_data.emplace_back(new interpolation_t(m_cubic_spline, new QLineSeries(this)));
  m_interpolation_data.emplace_back(new interpolation_t(m_hermite_spline, new QLineSeries(this)));
  m_interpolation_data.emplace_back(new interpolation_t(m_linear_interpolation, new QLineSeries(this)));

  create_chart();
  connect(m_points_importer, &import_points_t::points_are_ready, this, &MainWindow::update_points);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::create_chart()
{
  mp_axisX->setLabelFormat("%g");
  mp_axisY->setLabelFormat("%g");

  QChart* chart = ui->chart_widget->chart();
  chart->addAxis(mp_axisX, Qt::AlignBottom);
  chart->addAxis(mp_axisY, Qt::AlignLeft);

  m_data_series->setName("Input data");
  m_interpolation_data[it_cubic]->series->setName("Cubic");
  m_interpolation_data[it_cubic]->draw = ui->draw_cubic_checkbox->isChecked();
  m_interpolation_data[it_hermite]->series->setName("Hermite");
  m_interpolation_data[it_hermite]->draw = ui->draw_hermite_checkbox->isChecked();
  m_interpolation_data[it_linear]->series->setName("Linear");
  m_interpolation_data[it_linear]->draw = ui->draw_linear_checkbox->isChecked();

  chart->addSeries(m_data_series);
  m_data_series->attachAxis(mp_axisX);
  m_data_series->attachAxis(mp_axisY);

  for (auto& intepolation: m_interpolation_data) {
    chart->addSeries(intepolation->series);
    intepolation->series->attachAxis(mp_axisX);
    intepolation->series->attachAxis(mp_axisY);
  }

  ui->chart_widget->setRenderHint(QPainter::Antialiasing);
  ui->chart_widget->setRubberBand(QChartView::RectangleRubberBand);

  QHBoxLayout* header = new QHBoxLayout();
  header->addWidget(new QLabel("X", this));
  header->addWidget(new QLabel("Cubic", this));
  header->addWidget(new QLabel("Hermite", this));
  header->addWidget(new QLabel("Linear", this));
  ui->buttons_layout->addLayout(header);

  ui->xmin_spinbox->setValue(m_min_x);
  ui->xmax_spinbox->setValue(m_max_x);
  ui->xstep_spinbox->setValue(m_x_step);
  ui->spinbox_mark_limit->setValue(m_mark_limit);

  connect(mp_axisX, &QValueAxis::rangeChanged, this, &MainWindow::chart_was_zoomed);
  connect(mp_axisY, &QValueAxis::rangeChanged, this, &MainWindow::chart_was_zoomed);
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

  m_cubic_spline.set_points(a_correct_points.data(), correct_values.data(), a_correct_points.size());
  m_hermite_spline.set_points(a_correct_points.data(), correct_values.data(), a_correct_points.size());
  m_linear_interpolation.set_points(a_correct_points.data(), correct_values.data(), a_correct_points.size());

  calc_deviations();
}

double MainWindow::deviation(double a_real, double a_calculated)
{
  return (a_real - a_calculated) / a_calculated * 100;
}

void MainWindow::calc_deviations()
{
  for (auto& interp_data: m_interpolation_data) {
    interp_data->worst_point.clear();
  }

  size_t point_number = 0;
  for (auto &a_pair: m_points_map) {
    double real_value = a_pair.second;

    for (auto& interp_data: m_interpolation_data) {
      double interp_value = interp_data->interpolation(a_pair.first);
      double interp_deviation = deviation(real_value, interp_value);
      interp_data->deviation_labels[point_number]->setText(QString::number(interp_deviation));
      interp_data->deviation_labels[point_number]->setPalette(m_default_color);

      double pos_interp_deviation = abs(interp_deviation);
      interp_data->worst_point.add(pos_interp_deviation);

      if (pos_interp_deviation > m_mark_limit) {
        interp_data->deviation_labels[point_number]->setPalette(m_limit_color);
      }
    }
    point_number++;
  }
  for (auto& interp_data: m_interpolation_data) {
    interp_data->deviation_labels[interp_data->worst_point.get_index()]->setPalette(m_worst_color);
  }
}

void MainWindow::repaint_data_line()
{
  m_data_series->clear();
  for (size_t i = 0; i < m_x.size(); i++) {
    double value = m_y[i];
    if (m_draw_relative_points) {
      value = value / m_x[i];
    }
    m_min_y = m_min_y > value ? value : m_min_y;
    m_max_y = m_max_y < value ? value : m_max_y;
    m_data_series->append(m_x[i], value);
  }
}

void MainWindow::set_nice_axis_numbers(QValueAxis *a_axis, double a_min, double a_max, size_t a_ticks_count)
{
  double tick_interval = calc_chart_tick_interval(a_min, a_max, a_ticks_count);
  a_axis->setTickInterval(tick_interval);
  a_axis->setTickType(QValueAxis::TickType::TicksDynamic);
}

void MainWindow::draw_lines(double a_min, double a_max, double a_step)
{
  mp_axisX->setTickType(QValueAxis::TickType::TicksFixed);
  mp_axisY->setTickType(QValueAxis::TickType::TicksFixed);

  m_min_y = std::numeric_limits<double>::max();
  m_max_y = std::numeric_limits<double>::min();

  repaint_data_line();

  for (auto& interp: m_interpolation_data) {
    interp->series->clear();

    if (interp->draw) {
      for (double x = a_min; x < a_max; x += a_step) {
        double interpolation_value = interp->interpolation(x);
        if (m_draw_relative_points) {
          interpolation_value = interpolation_value / x;
        }
        m_min_y = m_min_y > interpolation_value ? interpolation_value : m_min_y;
        m_max_y = m_max_y < interpolation_value ? interpolation_value : m_max_y;
        interp->series->append(x, interpolation_value);
      }
    }
  }

  if (m_auto_scale) {
    mp_axisX->setRange(m_min_x, m_max_x);
    mp_axisY->setRange(m_min_y, m_max_y);
    set_new_zoom_start();
  }

  set_nice_axis_numbers(mp_axisX, mp_axisX->min(), mp_axisX->max(), m_tick_interval_count);
  set_nice_axis_numbers(mp_axisY, mp_axisY->min(), mp_axisY->max(), m_tick_interval_count);
}

std::tuple<double, double> get_double_power(double a_val)
{
  //Возвращает мантиссу и разряд base(10) экспоненты для double числа
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
  static constexpr std::array<double, 3> nice_numbers{ 1, 2, 5 };
  double bad_tick_interval = (a_max - a_min) / a_ticks_count;

  auto [val, power] = get_double_power(bad_tick_interval);

  auto nice_it = std::lower_bound(nice_numbers.begin(), nice_numbers.end(), val);

  if (nice_it != nice_numbers.begin()) {
    auto difference = val - *nice_it;
    auto next_difference = *(nice_it + 1) - val;
    if (next_difference < difference) nice_it--;
  }
  return *nice_it * pow(10, power);
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

void MainWindow::create_control(const vector<double>& a_x)
{
  for (size_t i = 0; i < a_x.size(); i++) {
    auto x = a_x[i];

    QCheckBox* point_checkbox = new QCheckBox(QString::number(x), this);
    if (std::find(m_correct_points.begin(), m_correct_points.end(), x) != m_correct_points.end()) {
      point_checkbox->setChecked(true);
    }
    connect(point_checkbox, &QCheckBox::clicked, this, &MainWindow::redraw_spline);

    QHBoxLayout* new_layout = new QHBoxLayout();
    new_layout->addWidget(point_checkbox);
    ui->buttons_layout->addLayout(new_layout);

    mp_point_checkboxes[i] = point_checkbox;
    mp_deviation_layouts[i] = new_layout;

    for (auto& interpolation: m_interpolation_data) {
      interpolation->deviation_labels[i] = new QLabel("0.00000", this);
      interpolation->deviation_labels[i]->setAutoFillBackground(true);
      mp_deviation_layouts[i]->addWidget(interpolation->deviation_labels[i]);
    }
  }
}

void MainWindow::reset_deviation_layouts()
{
  for (size_t i = 0; i < mp_deviation_layouts.size(); i++) {
    if (mp_deviation_layouts[i] != nullptr) delete mp_deviation_layouts[i];
    if (mp_point_checkboxes[i] != nullptr) delete mp_point_checkboxes[i];

    for (auto& interpolation: m_interpolation_data) {
      if (interpolation->deviation_labels[i] != nullptr) delete interpolation->deviation_labels[i];
    }
  }

  mp_deviation_layouts.resize(m_x.size());
  mp_point_checkboxes.resize(m_x.size());
  for (auto& interpolation: m_interpolation_data) {
    interpolation->deviation_labels.resize(m_x.size());
  }
}

void MainWindow::reinit_control_buttons()
{
  m_min_x = m_x.front();
  m_max_x = m_x.back();
  m_min_y = std::numeric_limits<double>::max();
  m_max_y = std::numeric_limits<double>::min();
  m_x_step = m_auto_step ? (m_x.back() - m_x.front()) / 400 : m_x_step;

  reset_deviation_layouts();
  create_control(m_x);
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

void MainWindow::update_points(vector<double> &a_x, vector<double> &a_y)
{
  input_data_error_t error = verify_data(a_x, a_y);

  switch(error) {
    case input_data_error_t::none: {
      m_x = std::move(a_x);
      m_y = std::move(a_y);

      reinit_control_buttons();
      repaint_spline();

      ui->xmin_spinbox->setValue(m_min_x);
      ui->xmax_spinbox->setValue(m_max_x);
      ui->xstep_spinbox->setValue(m_x_step);
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


void MainWindow::on_reset_scale_clicked()
{
  //Чтобы сброс проходил быстро
  mp_axisX->setTickInterval(m_max_x - m_min_x);
  mp_axisY->setTickInterval(m_max_y - m_min_y);

  mp_axisX->setRange(m_min_x, m_max_x);
  mp_axisY->setRange(m_min_y, m_max_y);
  repaint_spline();
  set_new_zoom_start();
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
  set_nice_axis_numbers(zoomed_axis, a_min, a_max, m_tick_interval_count);

  if (zoomed_axis == mp_axisY) {
    if (m_save_zoom) {
      //Коллбэк по Y всегда вызывается после коллбэка по X
      m_zoom_stack.push(QRectF(mp_axisX->min(), mp_axisX->max(), mp_axisY->min(), mp_axisY->max()));
    }
  }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *a_event)
{
  if (ui->chart_layout->geometry().intersects(QRect(a_event->pos(), QSize(1, 1)))) {
    if (!m_zoom_stack.empty()) {
      if (m_zoom_stack.top() != m_start_zoom) {
        m_zoom_stack.pop();

        const QRectF& field = m_zoom_stack.top();

        bool prev_auto_scale = m_auto_scale;
        m_auto_scale = false;
        //Чтобы в chart_was_zoomed ничего не пушилось в m_zoom_stack
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

void MainWindow::on_open_file_button_clicked()
{
  m_points_importer->create_import_points_dialog(this);
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


void MainWindow::set_new_zoom_start()
{
  m_start_zoom = { m_min_x, m_max_x, m_min_y, m_max_y };
  while(!m_zoom_stack.empty()) m_zoom_stack.pop();
  m_zoom_stack.push(m_start_zoom);
  m_save_zoom = true;
}

void MainWindow::on_spinbox_mark_limit_valueChanged(double a_value)
{
  m_mark_limit = a_value;
}

void MainWindow::on_draw_linear_checkbox_stateChanged(int a_state)
{
  m_interpolation_data[it_linear]->draw = a_state;
  repaint_spline();
}

void MainWindow::on_draw_cubic_checkbox_stateChanged(int a_state)
{
  m_interpolation_data[it_cubic]->draw = a_state;
  repaint_spline();
}

void MainWindow::on_draw_hermite_checkbox_stateChanged(int a_state)
{
  m_interpolation_data[it_hermite]->draw = a_state;
  repaint_spline();
}
