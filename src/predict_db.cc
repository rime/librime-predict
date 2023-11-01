#include "predict_db.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <darts.h>
#include <rime/resource.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/string_table.h>

namespace rime {

const string kPredictFormat = "Rime::Predict/1.0";
const string kPredictFormatPrefix = "Rime::Predict/";

bool PredictDb::Load() {
  LOG(INFO) << "loading predict db: " << file_name();

  if (IsOpen())
    Close();

  if (!OpenReadOnly()) {
    LOG(ERROR) << "error opening predict db '" << file_name() << "'.";
    return false;
  }

  metadata_ = Find<predict::Metadata>(0);
  if (!metadata_) {
    LOG(ERROR) << "metadata not found.";
    Close();
    return false;
  }

  if (!boost::starts_with(string(metadata_->format), kPredictFormatPrefix)) {
    LOG(ERROR) << "invalid metadata.";
    Close();
    return false;
  }

  if (!metadata_->key_trie) {
    LOG(ERROR) << "double array image not found.";
    Close();
    return false;
  }
  DLOG(INFO) << "found double array image of size " << metadata_->key_trie_size
             << ".";
  key_trie_->set_array(metadata_->key_trie.get(), metadata_->key_trie_size);

  if (!metadata_->value_trie) {
    LOG(ERROR) << "string table not found.";
    Close();
    return false;
  }
  DLOG(INFO) << "found string table of size " << metadata_->value_trie.get()
             << ".";
  value_trie_ = std::make_unique<StringTable>(metadata_->value_trie.get(),
                                              metadata_->value_trie_size);

  return true;
}

bool PredictDb::Save() {
  LOG(INFO) << "saving predict db: " << file_name();
  if (!key_trie_->total_size()) {
    LOG(ERROR) << "the trie has not been constructed!";
    return false;
  }
  return ShrinkToFit();
}

int PredictDb::WriteCandidates(const vector<predict::RawEntry>& candidates,
                               const table::Entry* entry) {
  auto* array = CreateArray<table::Entry>(candidates.size());
  auto* next = array->begin();
  for (size_t i = 0; i < candidates.size(); ++i) {
    *next++ = *entry++;
  }
  auto offset = reinterpret_cast<char*>(array) - address();
  return int(offset);
}

// build a temporary key_trie to get size
static int estimated_key_trie_image_size(size_t data_size, const char** keys) {
  Darts::DoubleArray dummy_key_trie;
  vector<int> dummy_values(data_size);
  dummy_key_trie.build(data_size, keys, NULL, dummy_values.data());
  return dummy_key_trie.total_size();
}

bool PredictDb::Build(const predict::RawData& data) {
  // create predict db
  int data_size = data.size();
  const size_t kReservedSize = 1024;

  size_t entry_count = 0;
  for (const auto& kv : data) {
    entry_count += kv.second.size();
  }
  StringTableBuilder string_table;
  vector<table::Entry> entries(entry_count);
  vector<const char*> keys;
  keys.reserve(data_size);
  int i = 0;
  for (const auto& kv : data) {
    if (kv.second.empty())
      continue;
    for (const auto& candidate : kv.second) {
      string_table.Add(candidate.text, candidate.weight,
                       &entries[i].text.str_id());
      entries[i].weight = float(candidate.weight);
      ++i;
    }
    keys.push_back(kv.first.c_str());
  }
  // this writes to entry vector, which should be copied to entry array later
  string_table.Build();
  size_t value_trie_image_size = string_table.BinarySize();
  size_t entry_array_size =
      data_size * (sizeof(Array<table::Entry>) - sizeof(table::Entry)) +
      entry_count * sizeof(table::Entry);
  size_t estimated_size =
      kReservedSize + entry_array_size +
      estimated_key_trie_image_size(data_size, keys.data()) +
      value_trie_image_size;
  if (!Create(estimated_size)) {
    LOG(ERROR) << "Error creating predict db file '" << file_name() << "'.";
    return false;
  }
  // create metadata in the beginning of file
  metadata_ = Allocate<predict::Metadata>();
  if (!metadata_) {
    LOG(ERROR) << "Error creating metadata in file '" << file_name() << "'.";
    return false;
  }

  // copy from entry vector to entry array
  const table::Entry* available_entries = &entries[0];
  vector<int> values;
  values.reserve(data_size);
  for (const auto& kv : data) {
    if (kv.second.empty())
      continue;
    values.push_back(WriteCandidates(kv.second, available_entries));
    available_entries += kv.second.size();
  }
  // build real key trie
  if (0 != key_trie_->build(data_size, keys.data(), NULL, values.data())) {
    LOG(ERROR) << "Error building double-array trie.";
    return false;
  }
  // save double-array image
  size_t key_trie_image_size = key_trie_->total_size();
  char* key_trie_image = Allocate<char>(key_trie_image_size);
  if (!key_trie_image) {
    LOG(ERROR) << "Error creating double-array image.";
    return false;
  }
  std::memcpy(key_trie_image, key_trie_->array(), key_trie_image_size);
  metadata_->key_trie = key_trie_image;
  // double-array size (number of units)
  metadata_->key_trie_size = key_trie_->size();
  // save string table
  char* value_trie_image = Allocate<char>(value_trie_image_size);
  if (!value_trie_image) {
    LOG(ERROR) << "Error creating value trie image.";
    return false;
  }
  string_table.Dump(value_trie_image, value_trie_image_size);
  metadata_->value_trie = value_trie_image;
  metadata_->value_trie_size = value_trie_image_size;
  // at last, complete the metadata
  std::strncpy(metadata_->format, kPredictFormat.c_str(),
               kPredictFormat.length());
  return true;
}

predict::Candidates* PredictDb::Lookup(const string& query) {
  int result = key_trie_->exactMatchSearch<int>(query.c_str());
  if (result == -1)
    return nullptr;
  else
    return Find<predict::Candidates>(result);
}

string PredictDb::GetEntryText(const ::rime::table::Entry& entry) {
  return value_trie_->GetString(entry.text.str_id());
}

}  // namespace rime
