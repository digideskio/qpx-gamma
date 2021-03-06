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
 *
 ******************************************************************************/

#ifndef WIDGET_PLOT2D_H
#define WIDGET_PLOT2D_H

#include <QWidget>
#include "qsquarecustomplot.h"
#include "daq_sink.h"
#include "peak2d.h"
#include "appearance.h"

namespace Ui {
class WidgetPlot2D;
}

class WidgetPlot2D : public QWidget
{
  Q_OBJECT

public:
  explicit WidgetPlot2D(QWidget *parent = 0);
  ~WidgetPlot2D();

  void update_plot(uint64_t sizex, uint64_t sizey, std::shared_ptr<Qpx::EntryList> spectrum_data);
  void set_axes(Qpx::Calibration cal_x, Qpx::Calibration cal_y, int bits, QString zlabel);
  void refresh();
  void replot_markers();
  void reset_content();

  void set_boxes(std::list<MarkerBox2D> boxes);
  void set_labels(std::list<MarkerLabel2D> labels);

  void set_scale_type(QString);
  void set_gradient(QString);
  void set_zoom(double);
  void set_show_legend(bool);
  void set_antialiased(bool);
  QString scale_type();
  QString gradient();
  double zoom();
  bool show_legend();

  void set_range_x(MarkerBox2D);

  std::list<MarkerBox2D> get_selected_boxes();
  std::list<MarkerLabel2D> get_selected_labels();

public slots:
  void zoom_out();

signals:
  void markers_set(Coord x, Coord y);
  void stuff_selected();

private slots:
  //void clicked_plottable(QCPAbstractPlottable*);
  void selection_changed();

  void plot_2d_mouse_upon(double x, double y);
  void plot_2d_mouse_clicked(double x, double y, QMouseEvent* event, bool channels);

  void optionsChanged(QAction*);
  void exportRequested(QAction*);
  void clicked_item(QCPAbstractItem*);

private:

  //gui stuff
  Ui::WidgetPlot2D *ui;
  QCPColorMap *colorMap;

  std::map<QString, QCPColorGradient> gradients_;
  QString current_gradient_;

  QMenu menuExportFormat;
  QMenu       menuOptions;
  bool show_gradient_scale_;

  std::map<QString, QCPAxis::ScaleType> scale_types_;
  QString current_scale_type_;

  QString Z_label_;

  std::list<MarkerBox2D> boxes_;
  std::list<MarkerLabel2D> labels_;
  MarkerBox2D range_;

  AppearanceProfile marker_looks;

  bool show_labels_, antialiased_;

  //scaling
  Qpx::Calibration calib_x_, calib_y_;
  int bits_;

  void build_menu();
  void toggle_gradient_scale();

  void setColorScheme(QColor fore, QColor back, QColor grid1, QColor grid2);
};

#endif // WIDGET_PLOT2D_H
