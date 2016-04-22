#ifndef GLOBALSEARCH_YOUTUBESEARCHPROVIDER_H
#define GLOBALSEARCH_YOUTUBESEARCHPROVIDER_H

#include "globalsearch/searchprovider.h"

class NetworkAccessManager;

class YoutubeSearchProvider : public SearchProvider {
  Q_OBJECT

 public:
  YoutubeSearchProvider(Application* app, QObject* parent = nullptr);

  void SearchAsync(int id, const QString& query) override;

 private:
  NetworkAccessManager* network_;
};

#endif  // GLOBALSEARCH_YOUTUBESEARCHPROVIDER_H
