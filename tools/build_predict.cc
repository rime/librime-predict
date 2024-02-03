//
// Copyright RIME Developers
//
#include <algorithm>
#include <iostream>
#include <rime/common.h>
#include "predict_db.h"

using namespace rime;

int main(int argc, char* argv[]) {
  rime::predict::RawData data;
  while (std::cin) {
    string key;
    std::cin >> key;
    if (key.empty())
      break;
    rime::predict::RawEntry entry;
    std::cin >> entry.text >> entry.weight;
    data[key].push_back(std::move(entry));
  }

  path file_path = argc > 1 ? path(argv[1]) : path{"predict.db"};
  PredictDb db(file_path);
  LOG(INFO) << "creating " << db.file_path();
  if (!db.Build(data) || !db.Save()) {
    LOG(ERROR) << "failed to build " << db.file_path();
    return 1;
  }
  LOG(INFO) << "created: " << db.file_path();
  return 0;
}
