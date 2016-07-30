#ifndef STREAMDISCOVERER_H
#define STREAMDISCOVERER_H

#include <QString>
#include <QObject>

#include <gst/pbutils/pbutils.h>

class StreamDiscoverer : public QObject {
  Q_OBJECT

 public:
  StreamDiscoverer();
  ~StreamDiscoverer();

 public slots:
  void discover(QString url);

  bool dataValid() const;
  const QString& url() const;
  const QString& format()
      const;  // This is localized, so only for human consumption.
  unsigned int bitrate() const;
  unsigned int depth() const;
  unsigned int channels() const;
  unsigned int sampleRate() const;

signals:
  void DiscoverererFinished();
  void DataReady();
  void Error(const QString& message);

 private:
  GstDiscoverer* discoverer_;

  QString url_;
  QString format_description_;
  unsigned int bitrate_;
  unsigned int channels_;
  unsigned int depth_;
  unsigned int sample_rate_;
  bool data_valid_;

  static const unsigned int kDiscoveryTimeoutS;

  // GstDiscoverer callbacks:
  static void on_discovered_cb(GstDiscoverer* discoverer,
                               GstDiscovererInfo* info, GError* err,
                               gpointer instance);
  static void on_finished_cb(GstDiscoverer* discoverer, gpointer instance);

  // Helper to return descriptive error messages:
  static QString gstDiscovererErrorMessage(GstDiscovererResult result);
};

#endif  // STREAMDISCOVERER_H
