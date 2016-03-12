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
 * Description:
 *      DialogSpectrumTemplate - edit dialog for spectrum template
 *      TableSpectraTemplates - table of spectrum templates
 *      DialogSpectraTemplates - dialog for managing spectrum templates
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/

#include "dialog_spectra_templates.h"
#include "ui_dialog_spectra_templates.h"
#include "ui_dialog_spectrum_template.h"
#include <QFileDialog>
#include <QMessageBox>

DialogSpectrumTemplate::DialogSpectrumTemplate(Qpx::Spectrum::Template newTemplate,
                                               std::vector<Qpx::Detector> current_dets,
                                               bool edit, QWidget *parent) :
  QDialog(parent),
  current_dets_(current_dets),
  attr_model_(this),
  ui(new Ui::DialogSpectrumTemplate)
{
  ui->setupUi(this);
  for (auto &q : Qpx::Spectrum::Factory::getInstance().types())
    ui->comboType->addItem(QString::fromStdString(q));

  ui->treeAttribs->setModel(&attr_model_);
  ui->treeAttribs->setItemDelegate(&attr_delegate_);
  ui->treeAttribs->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
//  attr_delegate_.eat_detectors(current_dets_);


  QRegExp rx("^\\w*$");
  QValidator *validator = new QRegExpValidator(rx, this);
  ui->lineName->setValidator(validator);

  if (edit) {
    myTemplate = newTemplate;
    ui->lineName->setEnabled(false);
    ui->comboType->setEnabled(false);
    Qpx::Spectrum::Template *newtemp = Qpx::Spectrum::Factory::getInstance().create_template(newTemplate.type);
    if (newtemp != nullptr) {
      myTemplate.description = newtemp->description;
      myTemplate.input_types = newtemp->input_types;
      myTemplate.output_types = newtemp->output_types;
    } else
      PL_WARN << "Problem with spectrum type. Factory cannot make template for " << newTemplate.type;
  } else {
    Qpx::Spectrum::Template *newtemp = Qpx::Spectrum::Factory::getInstance().create_template(ui->comboType->currentText().toStdString());
    if (newtemp != nullptr) {
      myTemplate = *newtemp;
      size_t sz = current_dets_.size();
      ui->spinDets->setValue(sz);
    } else
      PL_WARN << "Problem with spectrum type. Factory cannot make template for " << ui->comboType->currentText().toStdString();

    Qpx::Setting app = myTemplate.generic_attributes.branches.get(Qpx::Setting("appearance"));
    app.value_text = generateColor().name(QColor::HexArgb).toStdString();
    myTemplate.generic_attributes.branches.replace(app);
  }

  attr_model_.update(myTemplate.generic_attributes);

  updateData();
}

void DialogSpectrumTemplate::updateData() {

  ui->lineName->setText(QString::fromStdString(myTemplate.name_));
  ui->comboType->setCurrentText(QString::fromStdString(myTemplate.type));
  ui->spinBits->setValue(myTemplate.bits);
  ui->lineChannels->setText(QString::number(pow(2,myTemplate.bits)));

//  ui->spinDets->setValue(myTemplate.match_pattern.size());

  QString descr = QString::fromStdString(myTemplate.description) + "\n";
  if (myTemplate.output_types.size()) {
    descr += "\nOutput file types: ";
    for (auto &q : myTemplate.output_types) {
      descr += "*." + QString::fromStdString(q);
      if (q != myTemplate.output_types.back())
        descr += ", ";
    }
  }

  ui->labelDescription->setText(descr);
  attr_model_.update(myTemplate.generic_attributes);
}

DialogSpectrumTemplate::~DialogSpectrumTemplate()
{
  delete ui;
}

void DialogSpectrumTemplate::on_buttonBox_accepted()
{
  if (myTemplate.name_ == Qpx::Spectrum::Template().name_) {
    QMessageBox msgBox;
    msgBox.setText("Please give it a proper name");
    msgBox.exec();
  } else {
    PL_INFO << "Type requested " << myTemplate.type;
    myTemplate.generic_attributes = attr_model_.get_tree();
    Qpx::Spectrum::Spectrum *newSpectrum = Qpx::Spectrum::Factory::getInstance().create_from_template(myTemplate);
    if (newSpectrum == nullptr) {
      QMessageBox msgBox;
      msgBox.setText("Failed to create spectrum from template. Check requirements.");
      msgBox.exec();
    } else {
      delete newSpectrum;
      emit(templateReady(myTemplate));
      accept();
    }
  }
}

void DialogSpectrumTemplate::on_buttonBox_rejected()
{
  reject();
}

void DialogSpectrumTemplate::on_lineName_editingFinished()
{
  myTemplate.name_ = ui->lineName->text().toStdString();
}

void DialogSpectrumTemplate::on_spinBits_valueChanged(int arg1)
{
  myTemplate.bits = arg1;
  ui->lineChannels->setText(QString::number(pow(2,arg1)));
}

void DialogSpectrumTemplate::on_comboType_activated(const QString &arg1)
{
  Qpx::Spectrum::Template *newtemp = Qpx::Spectrum::Factory::getInstance().create_template(arg1.toStdString());
  if (newtemp != nullptr) {
    myTemplate = *newtemp;

    //keep these from previous
    myTemplate.bits = ui->spinBits->value();
    myTemplate.name_ = ui->lineName->text().toStdString();
//    myTemplate.match_pattern.resize(current_dets_.size());
//    myTemplate.add_pattern.resize(current_dets_.size());
    updateData();
  } else
    PL_WARN << "Problem with spectrum type. Factory refuses to make template for " << arg1.toStdString();
}

void DialogSpectrumTemplate::on_spinDets_valueChanged(int arg1)
{
  Qpx::Setting pattern;
  pattern = myTemplate.generic_attributes.branches.get(Qpx::Setting("pattern_coinc"));
  pattern.value_pattern.resize(arg1);
  myTemplate.generic_attributes.branches.replace(pattern);

  pattern = myTemplate.generic_attributes.branches.get(Qpx::Setting("pattern_anti"));
  pattern.value_pattern.resize(arg1);
  myTemplate.generic_attributes.branches.replace(pattern);

  pattern = myTemplate.generic_attributes.branches.get(Qpx::Setting("pattern_add"));
  pattern.value_pattern.resize(arg1);
  myTemplate.generic_attributes.branches.replace(pattern);

  updateData();
}





TableSpectraTemplates::TableSpectraTemplates(XMLableDB<Qpx::Spectrum::Template>& templates, QObject *parent)
  : QAbstractTableModel(parent),
    templates_(templates)
{
}

int TableSpectraTemplates::rowCount(const QModelIndex & /*parent*/) const
{
  return templates_.size();
}

int TableSpectraTemplates::columnCount(const QModelIndex & /*parent*/) const
{
  return 9;
}

QVariant TableSpectraTemplates::data(const QModelIndex &index, int role) const
{
  int row = index.row();
  int col = index.column();

  if (role == Qt::DisplayRole)
  {
    switch (col) {
    case 0:
      return QString::fromStdString(templates_.get(row).name_);
    case 1:
      return QString::fromStdString(templates_.get(row).type);
    case 2:
      return QString::number(templates_.get(row).bits);
    case 3:
      return QString::number(pow(2,templates_.get(row).bits));
    case 4:
      return QVariant::fromValue(templates_.get(row).generic_attributes.branches.get(Qpx::Setting("pattern_coinc")));
    case 5:
      return QVariant::fromValue(templates_.get(row).generic_attributes.branches.get(Qpx::Setting("pattern_anti")));
    case 6:
      return QVariant::fromValue(templates_.get(row).generic_attributes.branches.get(Qpx::Setting("pattern_add")));
    case 7:
      return QVariant::fromValue(templates_.get(row).generic_attributes.branches.get(Qpx::Setting("appearance")));
    case 8:
      return QVariant::fromValue(templates_.get(row).generic_attributes.branches.get(Qpx::Setting("visible")));
    }

  }
  return QVariant();
}

QVariant TableSpectraTemplates::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole)
  {
    if (orientation == Qt::Horizontal) {
      switch (section)
      {
      case 0:
        return QString("name");
      case 1:
        return QString("type");
      case 2:
        return QString("bits");
      case 3:
        return QString("channels");
      case 4:
        return QString("coinc");
      case 5:
        return QString("anti");
      case 6:
        return QString("add");
      case 7:
        return QString("appearance");
      case 8:
        return QString("default plot");
      }
    } else if (orientation == Qt::Vertical) {
      return QString::number(section);
    }
  }
  return QVariant();
}

void TableSpectraTemplates::update() {
  QModelIndex start_ix = createIndex( 0, 0 );
  QModelIndex end_ix = createIndex(templates_.size(), columnCount());
  emit dataChanged( start_ix, end_ix );
  emit layoutChanged();
}

Qt::ItemFlags TableSpectraTemplates::flags(const QModelIndex &index) const
{
  Qt::ItemFlags myflags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractTableModel::flags(index) & ~Qt::ItemIsSelectable;
  return myflags;
}






DialogSpectraTemplates::DialogSpectraTemplates(XMLableDB<Qpx::Spectrum::Template> &newdb,
                                               std::vector<Qpx::Detector> current_dets,
                                               QString savedir, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DialogSpectraTemplates),
  table_model_(newdb),
  selection_model_(&table_model_),
  templates_(newdb),
  current_dets_(current_dets),
  root_dir_(savedir)
{
  ui->setupUi(this);

  ui->spectraSetupView->setModel(&table_model_);
  ui->spectraSetupView->setItemDelegate(&special_delegate_);
  ui->spectraSetupView->setSelectionModel(&selection_model_);
  ui->spectraSetupView->verticalHeader()->hide();
  ui->spectraSetupView->horizontalHeader()->setStretchLastSection(true);
  ui->spectraSetupView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->spectraSetupView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->spectraSetupView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->spectraSetupView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->spectraSetupView->show();

  connect(&selection_model_, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(selection_changed(QItemSelection,QItemSelection)));
  connect(ui->spectraSetupView, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(selection_double_clicked(QModelIndex)));

  table_model_.update();
  ui->spectraSetupView->resizeColumnsToContents();
  ui->spectraSetupView->resizeRowsToContents();
}

DialogSpectraTemplates::~DialogSpectraTemplates()
{
  delete ui;
}

void DialogSpectraTemplates::selection_changed(QItemSelection, QItemSelection) {
  toggle_push();
}

void DialogSpectraTemplates::toggle_push() {
  if (selection_model_.selectedIndexes().empty()) {
    ui->pushEdit->setEnabled(false);
    ui->pushDelete->setEnabled(false);
    ui->pushUp->setEnabled(false);
    ui->pushDown->setEnabled(false);
  } else {
    ui->pushDelete->setEnabled(true);
  }

  if (selection_model_.selectedRows().size() == 1) {
    ui->pushEdit->setEnabled(true);
    QModelIndexList ixl = selection_model_.selectedRows();
    if (ixl.front().row() > 0)
      ui->pushUp->setEnabled(true);
    else
      ui->pushUp->setEnabled(false);
    if ((ixl.front().row() + 1) < templates_.size())
      ui->pushDown->setEnabled(true);
    else
      ui->pushDown->setEnabled(false);
  }

  if (templates_.empty())
    ui->pushExport->setEnabled(false);
  else
    ui->pushExport->setEnabled(true);

}

void DialogSpectraTemplates::on_pushImport_clicked()
{
  //ask clear or append?
  QString fileName = QFileDialog::getOpenFileName(this, "Load template spectra",
                                                  root_dir_, "Template set (*.tem)");
  if (validateFile(this, fileName, false)) {
    PL_INFO << "Reading templates from xml file " << fileName.toStdString();
    templates_.read_xml(fileName.toStdString());
    selection_model_.reset();
    table_model_.update();
    toggle_push();

    ui->spectraSetupView->horizontalHeader()->setStretchLastSection(true);
    ui->spectraSetupView->resizeColumnsToContents();
  }
}

void DialogSpectraTemplates::on_pushExport_clicked()
{
  QString fileName = CustomSaveFileDialog(this, "Save template spectra",
                                          root_dir_, "Template set (*.tem)");
  if (validateFile(this, fileName, true)) {
    PL_INFO << "Writing templates to xml file " << fileName.toStdString();
    templates_.write_xml(fileName.toStdString());
  }
}

void DialogSpectraTemplates::on_pushNew_clicked()
{
  Qpx::Spectrum::Template new_template;
//  new_template.appearance = generateColor().rgba();
//  new_template.match_pattern.resize(current_dets_.size());
//  new_template.add_pattern.resize(current_dets_.size());
  DialogSpectrumTemplate* newDialog = new DialogSpectrumTemplate(new_template, current_dets_, false, this);
  connect(newDialog, SIGNAL(templateReady(Qpx::Spectrum::Template)), this, SLOT(add_template(Qpx::Spectrum::Template)));
  newDialog->exec();
}

void DialogSpectraTemplates::on_pushEdit_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  int i = ixl.front().row();
  DialogSpectrumTemplate* newDialog = new DialogSpectrumTemplate(templates_.get(i), current_dets_, true, this);
  connect(newDialog, SIGNAL(templateReady(Qpx::Spectrum::Template)), this, SLOT(change_template(Qpx::Spectrum::Template)));
  newDialog->exec();
}

void DialogSpectraTemplates::selection_double_clicked(QModelIndex idx) {
  on_pushEdit_clicked();
}

void DialogSpectraTemplates::on_pushDelete_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;

  std::vector<std::string> names;

  for (auto &ix : ixl) {
    int i = ix.row();
    names.push_back(templates_.get(i).name_);
  }

  for (auto &q : names) {
    Qpx::Spectrum::Template t(q);
    templates_.remove_a(t);
  }
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}


void DialogSpectraTemplates::add_template(Qpx::Spectrum::Template newTemplate) {
  //notify if duplicate
  templates_.add(newTemplate);
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::change_template(Qpx::Spectrum::Template newTemplate) {
  templates_.replace(newTemplate);
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushSetDefault_clicked()
{
  //ask sure?
  templates_.write_xml(root_dir_.toStdString() + "/default_spectra.tem");
}

void DialogSpectraTemplates::on_pushUseDefault_clicked()
{
  //ask sure?
  templates_.clear();
  templates_.read_xml(root_dir_.toStdString() + "/default_spectra.tem");

  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushClear_clicked()
{
  templates_.clear();
  selection_model_.reset();
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushUp_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  templates_.up(ixl.front().row());
  selection_model_.setCurrentIndex(ixl.front().sibling(ixl.front().row()-1, ixl.front().column()),
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  table_model_.update();
  toggle_push();
}

void DialogSpectraTemplates::on_pushDown_clicked()
{
  QModelIndexList ixl = ui->spectraSetupView->selectionModel()->selectedRows();
  if (ixl.empty())
    return;
  templates_.down(ixl.front().row());
  selection_model_.setCurrentIndex(ixl.front().sibling(ixl.front().row()+1, ixl.front().column()),
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  table_model_.update();
  toggle_push();
}
