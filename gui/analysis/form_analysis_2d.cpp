/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      FormAnalysis2D -
 *
 ******************************************************************************/

#include "form_analysis_2d.h"
#include "ui_form_analysis_2d.h"
#include "fitter.h"
#include "qt_util.h"
#include "manip2d.h"
#include <QInputDialog>
#include <QSettings>


using namespace Qpx;

FormAnalysis2D::FormAnalysis2D(XMLableDB<Detector>& newDetDB, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormAnalysis2D),
  detectors_(newDetDB),
  my_gates_(nullptr),
  form_integration_(nullptr)
{
  ui->setupUi(this);

  initialized = false;

  my_gates_ = new FormMultiGates();
  connect(my_gates_, SIGNAL(gate_selected()), this, SLOT(update_peaks()));
  connect(my_gates_, SIGNAL(boxes_made()), this, SLOT(take_boxes()));
  connect(my_gates_, SIGNAL(range_changed(MarkerBox2D)), this, SLOT(update_range(MarkerBox2D)));

  form_integration_ = new FormIntegration2D();
  connect(form_integration_, SIGNAL(peak_selected()), this, SLOT(update_peaks()));
  connect(form_integration_, SIGNAL(range_changed(MarkerBox2D)), this, SLOT(update_range(MarkerBox2D)));

  connect(ui->plotMatrix, SIGNAL(marker_set(MarkerBox2D)), this, SLOT(update_gates(MarkerBox2D)));
  connect(ui->plotMatrix, SIGNAL(stuff_selected()), this, SLOT(matrix_selection()));

  ui->tabs->addTab(my_gates_, "Gates");
  ui->tabs->addTab(form_integration_, "Integration");
  ui->tabs->setCurrentWidget(my_gates_);

  loadSettings();


  res = 0;

  //connect(ui->widgetDetectors, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));

}

void FormAnalysis2D::take_boxes() {
  form_integration_->setPeaks(my_gates_->boxes());
}


void FormAnalysis2D::update_range(MarkerBox2D range) {
  ui->plotMatrix->set_range_x(range);
  ui->plotMatrix->replot_markers();
}


void FormAnalysis2D::matrix_selection() {
  //DBG << "User selected peaks in matrix";

  std::list<MarkerBox2D> chosen_peaks = ui->plotMatrix->get_selected_boxes();

  if (my_gates_->isVisible())
    my_gates_->choose_peaks(chosen_peaks);
  else if (form_integration_->isVisible())
    form_integration_->choose_peaks(chosen_peaks);

  update_peaks();
}


void FormAnalysis2D::configure_UI() {
  //while (ui->tabs->count())
//    ui->tabs->removeTab(0);

//  ui->plotSpectrum->setVisible(visible1);
//  ui->plotSpectrum->blockSignals(!visible1);

  ui->plotMatrix->set_gates_movable(false);

  ui->plotMatrix->set_gates_visible(false, true, false);

  ui->plotMatrix->set_show_boxes(true);


}

void FormAnalysis2D::setSpectrum(Project *newset, int64_t idx) {
  clear();
  spectra_ = newset;
  current_spectrum_ = idx;
  my_gates_->setSpectrum(newset, idx);
  form_integration_->setSpectrum(newset, idx);
}

void FormAnalysis2D::reset() {
  initialized = false;
//  while (ui->tabs->count())
//    ui->tabs->removeTab(0);
}

void FormAnalysis2D::initialize() {
  if (initialized)
    return;

  if (spectra_) {

    SinkPtr spectrum = spectra_->get_sink(current_spectrum_);

    if (spectrum) {
      Metadata md = spectrum->metadata();
      uint16_t bits = md.get_attribute("resolution").value_int;
      res = pow(2,bits);

      ui->plotMatrix->reset_content();
      ui->plotMatrix->setSpectra(*spectra_, current_spectrum_);
      ui->plotMatrix->update_plot();

      bool symmetrized = (md.get_attribute("symmetrized").value_int != 0);
    }
  }

  initialized = true;
  configure_UI();
}

void FormAnalysis2D::update_spectrum() {
  if (this->isVisible()) {
    ui->plotMatrix->update_plot();
  }
}

void FormAnalysis2D::showEvent( QShowEvent* event ) {
  QWidget::showEvent(event);

  QTimer::singleShot(50, this, SLOT(initialize()));
}

void FormAnalysis2D::update_peaks() {
  if (my_gates_->isVisible()) {
    MarkerBox2D box = my_gates_->current_gate().constraints;
    ui->plotMatrix->set_marker(box);
    ui->plotMatrix->set_boxes(my_gates_->current_peaks());
  } else if (form_integration_->isVisible()) {

    ui->plotMatrix->set_boxes(form_integration_->peaks());
  }

  ui->plotMatrix->replot_markers();
}

void FormAnalysis2D::update_gates(MarkerBox2D box) {
  Coord xx;
  if (box.visible)
    xx = box.x_c;

  Coord yy;
  if (box.visible)
    yy = box.y_c;

//  DBG << "marker from matrix " << xx.pos.energy() << " " << yy.pos.energy();

  if (my_gates_->isVisible())
    my_gates_->make_range(xx);
  else if (form_integration_->isVisible())
    form_integration_->make_range(xx, yy);
}

void FormAnalysis2D::closeEvent(QCloseEvent *event) {
  saveSettings();
  event->accept();
}

void FormAnalysis2D::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  settings_.beginGroup("AnalysisMatrix");
  ui->plotMatrix->set_gradient(settings_.value("gradient", "Hot").toString());
  ui->plotMatrix->set_scale_type(settings_.value("scale_type", "Logarithmic").toString());
  ui->plotMatrix->set_show_legend(settings_.value("show_legend", false).toBool());
  settings_.endGroup();

  if (my_gates_)
    my_gates_->loadSettings();
  if (form_integration_)
    form_integration_->loadSettings();
}

void FormAnalysis2D::saveSettings() {
  QSettings settings_;

  if (my_gates_)
    my_gates_->saveSettings();
  if (form_integration_)
    form_integration_->saveSettings();

  settings_.beginGroup("AnalysisMatrix");
  settings_.setValue("gradient", ui->plotMatrix->gradient());
  settings_.setValue("scale_type", ui->plotMatrix->scale_type());
  settings_.setValue("show_legend", ui->plotMatrix->show_legend());
  settings_.endGroup();
}

void FormAnalysis2D::clear() {
  res = 0;

  ui->plotMatrix->reset_content();

  current_spectrum_ = 0;

  if (my_gates_)
    my_gates_->clear();
  if (form_integration_)
    form_integration_->clear();
}

FormAnalysis2D::~FormAnalysis2D()
{
  delete ui;
}

void FormAnalysis2D::on_tabs_currentChanged(int index)
{
  ui->plotMatrix->set_marker(MarkerBox2D());
  update_peaks();
}
