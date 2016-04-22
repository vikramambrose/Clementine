#include "globalsearch/youtubesearchprovider.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>

#include "core/closure.h"
#include "core/logging.h"
#include "core/network.h"
#include "ui/iconloader.h"

namespace {
static const char* kApiKey = "AIzaSyCxdfVeZ8KSvIJlhOU9tXodGix_C2u3pH4";
static const char* kSearchUrl = "https://www.googleapis.com/youtube/v3/search";
}  // namespace

YoutubeSearchProvider::YoutubeSearchProvider(Application* app, QObject* parent)
  : SearchProvider(app, parent),
    network_(new NetworkAccessManager(this)) {
  SearchProvider::Init(
      "YouTube", "youtube", IconLoader::Load("soundcloud", IconLoader::Provider),
      WantsDelayedQueries | ArtIsProbablyRemote);
}

void YoutubeSearchProvider::SearchAsync(int id, const QString& query) {
  QUrlQuery url_query;
  url_query.addQueryItem("part", "snippet");
  url_query.addQueryItem("type", "video");
  url_query.addQueryItem("videoEmbeddable", "true");
  url_query.addQueryItem("videoSyndicated", "true");
  url_query.addQueryItem("q", query);
  url_query.addQueryItem("key", kApiKey);
  QUrl url(kSearchUrl);
  url.setQuery(url_query);

  qLog(Debug) << url;

  QNetworkReply* reply = network_->get(QNetworkRequest(url));
  NewClosure(reply, SIGNAL(finished()), [this, reply, id]() {
    reply->deleteLater();
    QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    QJsonArray items = json.object().value("items").toArray();

    ResultList results;

    for (QJsonValueRef item_ref : items) {
      QJsonObject item = item_ref.toObject();
      QJsonObject snippet = item.value("snippet").toObject();

      QString title = snippet.value("title").toString();

      QUrl url;
      url.setScheme("youtube");
      QString id = item.value("id").toObject().value("videoId").toString();
      url.setPath(id);

      QJsonArray thumbnails = snippet.value("thumbnails").toArray();
      QJsonArray::iterator thumbnail = std::max_element(
          thumbnails.begin(), thumbnails.end(), [](QJsonValue a, QJsonValue b) {
        QJsonObject a_object = a.toObject();
        QJsonObject b_object = b.toObject();
        int size_a = a_object.value("width").toInt(0) * a_object.value("height").toInt(0);
        int size_b = b_object.value("width").toInt(0) * b_object.value("height").toInt(0);
        return size_a < size_b;
      });

      QString art_url = thumbnail->toObject().value("url").toString();


      Result result(this);
      result.metadata_.set_title(title);
      result.metadata_.set_url(url);
      result.metadata_.set_art_automatic(art_url);
      results << result;
    }

    emit ResultsAvailable(id, results);

    emit SearchFinished(id);
  });
}
