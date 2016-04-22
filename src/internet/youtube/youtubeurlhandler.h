#ifndef INTERNET_YOUTUBE_YOUTUBEURLHANDLER_H_
#define INTERNET_YOUTUBE_YOUTUBEURLHANDLER_H_

#include "core/urlhandler.h"
#include "ui/iconloader.h"

class YoutubeUrlHandler : public UrlHandler {
  Q_OBJECT

 public:
  explicit YoutubeUrlHandler(QObject* parent = nullptr);

  QString scheme() const override { return "youtube"; }

  QIcon icon() const override {
    return IconLoader::Load("skydrive", IconLoader::Provider);
  }

  LoadResult StartLoading(const QUrl& url) override;
};

#endif  // INTERNET_YOUTUBE_YOUTUBEURLHANDLER_H_
