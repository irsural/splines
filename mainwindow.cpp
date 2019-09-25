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
  m_freq {
/*
    40,43,45,47,51,53,55,58,60,62,66,68,70,73,75,78,82,85,88,90,95,
    100,107,115,120,125,130,140,150,170,190,220,245,280,310,350,400,
    500,630,800,1000,1200,1400,1600,1800,2000
*/

    40,43,47,51,55,60,70,80,90,100,120,150,200,270,300,400,500,600,700,
    800,900,1000,1100,1200,1300,1400,1550,1700,1850,2000

/*
    0.005, 0.01, 0.012, 0.015, 0.017, 0.02, 0.022, 0.025, 0.03, 0.035,
    0.04, 0.045, 0.05, 0.055, 0.06, 0.065, 0.07, 0.075, 0.08, 0.085,
    0.09, 0.095, 0.1
*/
  },
  m_current {
/*
    1.124160724,1.123531515,1.123305434,
    1.123028894,1.122647062,1.122525678,1.122329487,1.122114651,
    1.121953222,1.121781552,1.121655044,1.121583428,1.121435291,
    1.121308576,1.12120405,1.121133936,1.120987367,1.120859157,
    1.120751864,1.12069737,1.12059512,1.120506705,1.120372833,
    1.120263564,1.120198705,1.120142823,1.120052362,1.119981992,
    1.119960749,1.119886246,1.119829218,1.119790066,1.119763047,
    1.11968431306542 ,1.119676734,1.119676047,1.119678627,1.119670334,
    1.119779062,1.119859467,1.119912178,1.120020336,1.120075987,
    1.120179551,1.12026671,1.120341176
*/

    0.0975659955342365,0.0975275796083106,0.0974902362425586,0.0974626799653618,
    0.0974428273311505,0.0974190078860744,0.0973867728755724,0.0973670305493144,
    0.097356562748723,0.0973469938187897,0.0973328500162498,0.0973229205373056,
    0.0973166424972046,0.0973116992231381,0.0973109104638724,0.0973123826935796,
    0.0973161706285211,0.0973225522548119,0.0973278484591096,0.0973360484177552,
    0.0973427546505328,0.0973505326522316,0.0973620306676114,0.0973731875398392,
    0.0973816450679029,0.0973955016326368,0.0974166999863728,0.0974368623084973,
    0.0974635746790084,0.0974903935919296

/*
    0.00487195071094411, 0.0097434035308675, 0.0116916480232211, 0.014614234246976,
    0.0165628298467953, 0.0194855261107702, 0.021434516567259, 0.0243574436987256,
    0.0292289525344483, 0.0341005974305858, 0.0389726369759468, 0.0438446322421415,
    0.0487166893668983, 0.0535886454321949, 0.0584610662874094, 0.0633339463601836,
    0.0682064924543303, 0.0730788352069786, 0.0779509458907587, 0.0828234277541804,
    0.0876964685548731, 0.0925689873120506, 0.097441018511837
*/
    /*
    0.004869778,0.009739344,0.011686854,0.014609232,0.016556957,0.019478652,
    0.021426602,0.024348174,0.029217819,0.034087368,0.038957179,0.043826781,
    0.048696631,0.053566454,0.058436382,0.063306662,0.06817663,0.073046623,
    0.077916614,0.082786384,0.087656405,0.092525789,0.097395678
*/
  },
  m_correct_points {

    40, 47, 70, 120, 300, 1000, 1400, 2000

/*
    0.005, 0.03, 0.1
*/
  },
  m_points(),
  mp_chart(new QChart()),
  mp_chart_view(new QChartView(mp_chart, this)),
  m_cubic_spline(),
  m_hermite_spline(),
  mp_linear_data(new QLineSeries(this)),
  mp_cubic_data(new QLineSeries(this)),
  mp_hermite_data(new QLineSeries(this)),
  mp_axisX(new QValueAxis(this)),
  m_min_x(m_freq[0]),
  m_max_x(m_freq[m_freq.size() - 1]),
  mp_axisY(new QValueAxis(this)),
  m_min_y(0),
  m_max_y(0),
  m_x_step((m_freq[m_freq.size() - 1] - m_freq[0]) / 400),
  m_auto_step(true),
  m_auto_scale(true),
  m_draw_linear(true),
  m_draw_cubic(false),
  m_draw_hermite(true),
  mp_layouts(m_freq.size()),
  mp_point_checkboxes(m_freq.size()),
  mp_diff_cubic_labels(m_freq.size()),
  mp_diff_hermite_labels(m_freq.size()),
  m_better_color(QPalette::Window, QColor(0xbfffbd)),
  m_worst_color(QPalette::Window, QColor(0xfaa88e)),
  m_default_color(),
  m_draw_relative_points(false),
  m_points_importer(new import_points_t(this))
{
  ui->setupUi(this);

  create_chart();
  create_control(m_freq);

  m_points_importer->set_correct_points(m_correct_points);
  connect(m_points_importer, &import_points_t::points_are_ready,
    this, &MainWindow::update_points);

  m_cubic_spline.set_boundary(tk::spline::second_deriv, 0,
    tk::spline::second_deriv, 0, false);
  repaint_spline();
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::create_chart()
{
  mp_chart->legend()->setAlignment(Qt::AlignBottom);
  mp_hermite_data->setName("hermite");
  mp_linear_data->setName("linear");
  mp_cubic_data->setName("cubic");

  mp_axisX->setLabelFormat("%f");
  mp_chart->addAxis(mp_axisX, Qt::AlignBottom);
  mp_axisY->setLabelFormat("%f");
  mp_chart->addAxis(mp_axisY, Qt::AlignLeft);

  mp_chart_view->setRenderHint(QPainter::Antialiasing);
  mp_chart_view->setRubberBand(QChartView::RectangleRubberBand);
  ui->chart_layout->addWidget(mp_chart_view);

  QHBoxLayout* header = new QHBoxLayout();
  ui->buttons_layout->addLayout(header);
  header->addWidget(new QLabel("Frequency", this));
  header->addWidget(new QLabel("Cubic", this));
  header->addWidget(new QLabel("Hermite", this));

  ui->xmin_spinbox->setValue(m_min_x);
  ui->xmax_spinbox->setValue(m_max_x);
  ui->xstep_spinbox->setValue(m_x_step);
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
    correct_values.data(), a_points.size());

  calc_difs();
}

void MainWindow::calc_difs()
{
  size_t worst_cubic_ind = 0;
  double worst_cubic_val = 0;
  size_t worst_hermite_ind = 0;
  double worst_hermite_val = 0;

  size_t label_ind = 0;
  for (auto a_pair: m_points) {
    double real_value = a_pair.second;

    double cubic_value = m_cubic_spline(a_pair.first);
    double cubic_diff = (real_value - cubic_value) / cubic_value * 100;

    double hermite_value = m_hermite_spline(a_pair.first);
    double hermite_diff = (real_value - hermite_value) / hermite_value * 100;

    mp_diff_cubic_labels[label_ind]->setText(QString::number(cubic_diff));
    mp_diff_hermite_labels[label_ind]->setText(QString::number(hermite_diff));

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

    if (worst_cubic_val < abs(cubic_diff)) {
      worst_cubic_val = abs(cubic_diff);
      worst_cubic_ind = label_ind;
    }
    if (worst_hermite_val < abs(hermite_diff)) {
      worst_hermite_val = abs(hermite_diff);
      worst_hermite_ind = label_ind;
    }

    label_ind++;
  }
  mp_diff_cubic_labels[worst_cubic_ind]->setPalette(m_worst_color);
  mp_diff_hermite_labels[worst_hermite_ind]->setPalette(m_worst_color);
}

void MainWindow::draw_lines(double a_min, double a_max, double a_step)
{
  mp_cubic_data->clear();
  mp_hermite_data->clear();
  mp_linear_data->clear();

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
    mp_chart->addSeries(mp_cubic_data);
    mp_chart->addSeries(mp_hermite_data);
    mp_chart->addSeries(mp_linear_data);

    mp_cubic_data->attachAxis(mp_axisX);
    mp_cubic_data->attachAxis(mp_axisY);
    mp_hermite_data->attachAxis(mp_axisX);
    mp_hermite_data->attachAxis(mp_axisY);
    mp_linear_data->attachAxis(mp_axisX);
    mp_linear_data->attachAxis(mp_axisY);
    drawed = true;
  }
  if (m_auto_scale) {
    mp_axisY->setRange(m_min_y, m_max_y);
    mp_axisX->setRange(a_min, a_max);
  }
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
  }

  m_min_x = m_freq[0];
  m_max_x = m_freq[m_freq.size() - 1];
  m_min_y = 0;
  m_max_y = 0;
  m_x_step = (m_freq[m_freq.size() - 1] - m_freq[0]) / 400;
  mp_layouts.resize(m_freq.size());
  mp_point_checkboxes.resize(m_freq.size());
  mp_diff_cubic_labels.resize(m_freq.size());
  mp_diff_hermite_labels.resize(m_freq.size());

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
  calc_splines(m_correct_points);
  draw_lines(m_min_x, m_max_x, m_x_step);
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
  if (a_event->text() == "w") {
    move_direction = import_points_t::move_direction_t::up;
  } else if (a_event->text() == "s") {
    move_direction = import_points_t::move_direction_t::down;
  } else if (a_event->text() == "a") {
    move_direction = import_points_t::move_direction_t::left;
  } else if (a_event->text() == "d") {
    move_direction = import_points_t::move_direction_t::right;
  }
  if (move_direction != import_points_t::move_direction_t::none) {
    double current_x = m_points_importer->get_next_data(move_direction);
    ui->current_x_label->setText(QString::number(current_x));
  }
}
