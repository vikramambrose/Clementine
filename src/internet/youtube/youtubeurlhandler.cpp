#include "youtubeurlhandler.h"

#include <QFile>
#include <QSize>
#include <QWebEngineView>

#include "core/logging.h"

YoutubeUrlHandler::YoutubeUrlHandler(QObject* parent)
    : UrlHandler(parent) {}

UrlHandler::LoadResult YoutubeUrlHandler::StartLoading(const QUrl& url) {
  QWebEngineView* web_view = new QWebEngineView;
  web_view->page()->profile()->installUrlSchemeHandler(
      "youtube", new YoutubeUrlSchemeHandler);
  QUrl load_url("qrc:/youtube/youtube.html");
  load_url.setFragment(url.path());
  web_view->load(load_url);
  web_view->setMinimumSize(QSize(640, 390));
  web_view->show();
  return LoadResult(url, LoadResult::NoMoreTracks);
}
