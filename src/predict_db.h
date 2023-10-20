#ifndef RIME_PREDICT_DB_H_
#define RIME_PREDICT_DB_H_

#include <darts.h>
#include <rime/resource.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/string_table.h>
#include <rime/dict/table.h>

namespace rime {

namespace predict {

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t db_checksum;
  OffsetPtr<char> key_trie;  // DoubleArray (query -> offset of Candidates)
  uint32_t key_trie_size;
  OffsetPtr<char> value_trie;  // StringTable
  uint32_t value_trie_size;
};

using Candidates = ::rime::Array<::rime::table::Entry>;

struct RawEntry {
  string text;
  double weight;
};

using RawData = map<string, vector<RawEntry>>;

}  // namespace predict

class PredictDb : public MappedFile {
 public:
  PredictDb(const string& file_name)
      : MappedFile(file_name),
        key_trie_(new Darts::DoubleArray),
        value_trie_(new StringTable) {}

  bool Load();
  bool Save();
  bool Build(const predict::RawData& data);
  predict::Candidates* Lookup(const string& query);
  string GetEntryText(const ::rime::table::Entry& entry);

 private:
  int WriteCandidates(const vector<predict::RawEntry>& candidates,
                      StringTableBuilder* string_table,
                      table::Entry*& entry);

  predict::Metadata* metadata_ = nullptr;
  the<Darts::DoubleArray> key_trie_;
  the<StringTable> value_trie_;
};

}  // namespace rime

#endif  // RIME_PREDICT_DB_H_
