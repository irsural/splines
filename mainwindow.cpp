#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>
#include <limits>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  m_freq(),
  m_current(),
  m_correct_points { 40, 47, 70, 120, 300, 1000, 1400, 2000 },
  m_points(),
//  mp_chart(new QChart()),
//  mp_chart_view(new QChartView(mp_chart, this)),
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
  mp_layouts(m_freq.size()),
  mp_point_checkboxes(m_freq.size()),
  mp_diff_cubic_labels(m_freq.size()),
  mp_diff_hermite_labels(m_freq.size()),
  mp_diff_linear_labels(m_freq.size()),
  m_better_color(QPalette::Window, QColor(0xbfffbd)),
  m_worst_color(QPalette::Window, QColor(0xfaa88e)),
  m_limit_color(QPalette::Window, QColor(0xffe9d1)),
  m_default_color(),
  m_draw_relative_points(false),
  m_points_importer(new import_points_t(this))
{
  ui->setupUi(this);

  create_chart();

  connect(m_points_importer, &import_points_t::points_are_ready,
    this, &MainWindow::update_points);

  m_cubic_spline.set_boundary(tk::spline::second_deriv, 0,
    tk::spline::second_deriv, 0, false);

  m_points_importer->set_correct_points(m_correct_points);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::create_chart()
{
  mp_hermite_data->setName("hermite");
  mp_linear_data->setName("linear");
  mp_cubic_data->setName("cubic");

  QChart* chart = ui->chart_widget->chart();

  mp_axisX->setLabelFormat("%g");
  chart->addAxis(mp_axisX, Qt::AlignBottom);
  mp_axisY->setLabelFormat("%g");
  chart->addAxis(mp_axisY, Qt::AlignLeft);

  ui->chart_widget->setRenderHint(QPainter::Antialiasing);
  ui->chart_widget->setRubberBand(QChartView::RectangleRubberBand);
//  ui->chart_layout->addWidget(mp_chart_view);


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


void MainWindow::calc_splines(vector<double> &a_points)
{
  assert(m_freq.size() == m_current.size());

  m_points.clear();
  for (size_t i = 0; i < m_freq.size(); i++) {
    m_points[m_freq[i]] = m_current[i];
  }

  vector<double> correct_values;
  for (auto point: a_points) {
    auto value = m_points.find(point);
    if (value != m_points.end()) {
      correct_values.push_back(value->second);
    }
  }

  m_cubic_spline.set_points(a_points, correct_values);
  m_hermite_spline.set_points(a_points.data(),
    correct_values.data(), int(a_points.size()));

  m_linear_interpolation.clear();
  m_linear_interpolation.assign(a_points.data(), correct_values.data(),
    int(a_points.size()));

  calc_difs();
}

void MainWindow::calc_difs()
{
  size_t worst_cubic_ind = 0;
  double worst_cubic_val = 0;
  size_t worst_hermite_ind = 0;
  double worst_hermite_val = 0;
  size_t worst_linear_ind = 0;
  double worst_linear_val = 0;

  size_t label_ind = 0;
  for (auto a_pair: m_points) {
    double real_value = a_pair.second;

    double cubic_value = m_cubic_spline(a_pair.first);
    double cubic_diff = (real_value - cubic_value) / cubic_value * 100;

    double hermite_value = m_hermite_spline(a_pair.first);
    double hermite_diff = (real_value - hermite_value) / hermite_value * 100;

    double linear_value = m_linear_interpolation.calc(a_pair.first);
    double linear_diff = (real_value - linear_value) / linear_value * 100;

    mp_diff_cubic_labels[label_ind]->setText(QString::number(cubic_diff));
    mp_diff_hermite_labels[label_ind]->setText(QString::number(hermite_diff));
    mp_diff_linear_labels[label_ind]->setText(QString::number(linear_diff));

    if (abs(cubic_diff) < abs(hermite_diff)) {
      mp_diff_cubic_labels[label_ind]->setPalette(m_better_color);
      mp_diff_hermite_labels[label_ind]->setPalette(m_default_color);
    } else if (abs(cubic_diff) > abs(hermite_diff)) {
      mp_diff_cubic_labels[label_ind]->setPalette(m_default_color);
      mp_diff_hermite_labels[label_ind]->setPalette(m_better_color);
    } else {
      mp_diff_cubic_labels[label_ind]->setPalette(m_default_color);
      mp_diff_hermite_labels[label_ind]->setPalette(m_default_color);
    }
    mp_diff_linear_labels[label_ind]->setPalette(m_default_color);

    if (worst_cubic_val < abs(cubic_diff)) {
      worst_cubic_val = abs(cubic_diff);
      worst_cubic_ind = label_ind;
    }
    if (worst_hermite_val < abs(hermite_diff)) {
      worst_hermite_val = abs(hermite_diff);
      worst_hermite_ind = label_ind;
    }
    if (worst_linear_val < abs(linear_diff)) {
      worst_linear_val = abs(linear_diff);
      worst_linear_ind = label_ind;
    }

    if (abs(cubic_diff) > ui->spinbox_mark_limit->value()) {
      mp_diff_cubic_labels[label_ind]->setPalette(m_limit_color);
    }
    if (abs(hermite_diff) > ui->spinbox_mark_limit->value()) {
      mp_diff_hermite_labels[label_ind]->setPalette(m_limit_color);
    }

    label_ind++;
  }
  mp_diff_cubic_labels[worst_cubic_ind]->setPalette(m_worst_color);
  mp_diff_hermite_labels[worst_hermite_ind]->setPalette(m_worst_color);
  mp_diff_linear_labels[worst_linear_ind]->setPalette(m_worst_color);
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

  double relative_point = m_points[m_correct_points[0]] / m_correct_points[0];

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

  for (size_t i = 0; i < m_freq.size(); i++) {
    double value = m_current[i];
    if (m_draw_relative_points) {
      value = (value/m_freq[i] - relative_point) * 100;
    }
    if (m_draw_linear) {
      m_min_y = m_min_y > value ? value : m_min_y;
      m_max_y = m_max_y < value ? value : m_max_y;
      mp_linear_data->append(m_freq[i], value);
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
  }

  double x_tick_interval = calc_chart_tick_interval(mp_axisX->min(),
    mp_axisX->max(), ui->spinbox_ticks_count->value());
  mp_axisX->setTickInterval(x_tick_interval);
  mp_axisX->setTickType(QValueAxis::TickType::TicksDynamic);
  double y_tick_interval = calc_chart_tick_interval(mp_axisY->min(),
    mp_axisY->max(), ui->spinbox_ticks_count->value());
  mp_axisY->setTickInterval(y_tick_interval);
  mp_axisY->setTickType(QValueAxis::TickType::TicksDynamic);
}

std::tuple<double, double> get_double_power(double a_val)
{
  double value = 0;
  double power = 0;
  if (a_val > std::numeric_limits<double>::epsilon()) {
    power = /*1 + */int(std::floor(std::log10(std::fabs(a_val))));
    value = a_val * std::pow(10 , -1*power);
  }
  return std::make_tuple(value, power);
}

double MainWindow::calc_chart_tick_interval(double a_min, double a_max,
  size_t a_ticks_count)
{
  std::array<double, 3> nice_numbers{ 1, 2, 5 };
  double bad_tick_interval = (a_max - a_min) / a_ticks_count;

  auto double_info = get_double_power(bad_tick_interval);
  double val = std::get<0>(double_info);
  double power = std::get<1>(double_info);

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

  double relative_point = m_points[m_correct_points[0]] / m_correct_points[0];

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

  double relative_point = m_points[m_correct_points[0]] / m_correct_points[0];

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
  a_x.reserve(m_freq.size());
  a_y.reserve(m_freq.size());

  double relative_point = m_points[m_correct_points[0]] / m_correct_points[0];

  for (size_t i = 0; i < m_freq.size(); i++) {
    double value = m_current[i];
    if (m_draw_relative_points) {
      value = (value/m_freq[i] - relative_point) * 100;
    }
    a_x.push_back(m_freq[i]);
    a_y.push_back(value);
  }
}

void MainWindow::draw_line(const vector<double>& a_x,
  const vector<double> &a_y, QLineSeries* a_series)
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
  m_points_importer->set_correct_points(m_correct_points);
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

  m_min_x = m_freq[0];
  m_max_x = m_freq[m_freq.size() - 1];
  m_min_y = std::numeric_limits<double>::max();
  m_max_y = std::numeric_limits<double>::min();
  m_x_step = m_auto_step ? (m_freq[m_freq.size() - 1] - m_freq[0]) / 400 : m_x_step;
  mp_layouts.resize(m_freq.size());
  mp_point_checkboxes.resize(m_freq.size());
  mp_diff_cubic_labels.resize(m_freq.size());
  mp_diff_hermite_labels.resize(m_freq.size());
  mp_diff_linear_labels.resize(m_freq.size());

  ui->xmin_spinbox->setValue(m_min_x);
  ui->xmax_spinbox->setValue(m_max_x);
  ui->xstep_spinbox->setValue(m_x_step);

  create_control(m_freq);
}

void MainWindow::update_points(vector<double> a_x, vector<double> a_y,
  vector<double> a_correct_points)
{
  m_freq = std::move(a_x);
  m_current = std::move(a_y);
  m_correct_points = std::move(a_correct_points);

  reinit_control_buttons();
  repaint_spline();

  m_points_importer->get_next_data(import_points_t::move_direction_t::none);
}

void MainWindow::repaint_spline()
{
  if (m_freq.size()) {
//    vector<double> x;
//    vector<double> y;

//    if (m_draw_linear) {
//      fill_linear(x, y);
//      draw_line(x, y, mp_linear_data);
//    }

    calc_splines(m_correct_points);
    draw_lines(m_min_x, m_max_x, m_x_step);

//    if (m_draw_cubic) {
//      fill_cubic(x, y, m_min_x, m_max_x, m_x_step);
//      draw_line(x, y, mp_cubic_data);
//    }

//    if (m_draw_hermite) {
//      fill_hermite(x, y, m_min_x, m_max_x, m_x_step);
//      draw_line(x, y, mp_hermite_data);
//    }
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
  //Чтобы сброс проходил быстро
  mp_axisX->setTickInterval(m_max_x - m_min_x);
  mp_axisY->setTickInterval(m_max_y - m_min_y);

  mp_axisX->setRange(m_min_x, m_max_x);
  mp_axisY->setRange(m_min_y, m_max_y);
  repaint_spline();
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
  import_points_t::move_direction_t move_direction =
    import_points_t::move_direction_t::none;
  qDebug() << a_event->nativeScanCode();
  if (a_event->nativeScanCode() == 17/*w*/) {
    move_direction = import_points_t::move_direction_t::up;
  } else if (a_event->nativeScanCode() == 31/*s*/) {
    move_direction = import_points_t::move_direction_t::down;
  } else if (a_event->nativeScanCode() == 30/*a*/) {
    move_direction = import_points_t::move_direction_t::left;
  } else if (a_event->nativeScanCode() == 32/*d*/) {
    move_direction = import_points_t::move_direction_t::right;
  }
  if (move_direction != import_points_t::move_direction_t::none) {
    double current_x = m_points_importer->get_next_data(move_direction);
    ui->current_x_label->setText(QString::number(current_x));
  }
}

void MainWindow::on_show_all_graps_button_clicked()
{
//  vector<QLineSeries*> linear_series;
//  int graphs_count = m_points_importer->is_rows_selected() ?
//    m_points_importer->get_rows_count() : m_points_importer->get_cols_count();

//  for (int i = 0; i < graphs_count; i++) {
//    vector<double> x;
//    vector<double> y;
//    m_points_importer->get_data(x, y, i);
//    linear_series[i] = new QLineSeries(this);
//    draw_line(x, y, linear_series[i]);
//  }
}

void MainWindow::on_button_apply_ticks_count_clicked()
{
  m_auto_scale = false;
  repaint_spline();
  m_auto_scale = true;
}

void MainWindow::chart_was_zoomed(qreal a_min, qreal a_max)
{
  QObject* obj = QObject::sender();
  QValueAxis* zoomed_axis = qobject_cast<QValueAxis*>(obj);

  if (zoomed_axis == mp_axisX) qDebug() << "its X";
  if (zoomed_axis == mp_axisY) qDebug() << "its Y";

  double tick_interval = calc_chart_tick_interval(a_min, a_max,
    ui->spinbox_ticks_count->value());
  zoomed_axis->setTickInterval(tick_interval);
  zoomed_axis->setTickType(QValueAxis::TickType::TicksDynamic);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *a_event)
{
  if (ui->chart_layout->geometry().intersects(
    QRect(a_event->pos(), QSize(1, 1))))
  {
    qDebug() << "click hit";
  }
}
