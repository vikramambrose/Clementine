#ifndef STREAMDISCOVERER_H
#define STREAMDISCOVERER_H

#include <QString>
#include <QObject>

class StreamDiscoverer : public QObject {
  Q_OBJECT

 public:
  StreamDiscoverer(QString url);
  ~StreamDiscoverer();

 public slots:
  bool discoveryValid() const;
  const QString& url() const;
  const QString& format()
      const;  // This is localized, so only for human consumption.
  unsigned int bitrate() const;
  unsigned int depth() const;
  unsigned int channels() const;
  unsigned int sampleRate() const;

 private:
  QString url_;
  QString format_description_;

  unsigned int bitrate_;
  unsigned int channels_;
  unsigned int depth_;
  unsigned int sample_rate_;
  bool data_valid_;

  static const unsigned int kDiscoveryTimeoutS;
};

#endif  // STREAMDISCOVERER_H
