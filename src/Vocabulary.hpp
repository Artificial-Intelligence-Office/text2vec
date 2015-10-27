#include "text2vec.h"
using namespace Rcpp;
using namespace std;
//
// vector<string> get_ngrams(const CharacterVector terms, int ngram_min, int ngram_max, const string ngram_delim = "_") {
//   // iterates through input vector by window of size = n_max and build n-grams
//   // for terms ["a", "b", "c", "d"] and n_min = 1, n_max = 2
//   // will build 1:3-grams in following order
//   //"a"     "a_b"   "a_b_c" "b"     "b_c"   "b_c_d" "c"     "c_d"   "d"
//
//   size_t len = terms.size();
//
//   // calculate res size
//   size_t out_len = 0;
//   if(len >= ngram_min)
//     for(int i = ngram_min; i <= ngram_max; i++)
//       out_len += (len - i) + 1;
//   vector< string> res(out_len);
//
//   string k_gram;
//   size_t k, i = 0, last_observed;
//   for(size_t j = 0; j < len; j ++ ) {
//     k = 0;
//     last_observed = j + k;
//     while (k < ngram_max && last_observed < len) {
//       if( k == 0) {
//         k_gram = terms[last_observed];
//       }
//       else
//         k_gram = k_gram + ngram_delim + terms[last_observed];
//       if(k >= ngram_min - 1) {
//         res[i] = k_gram;
//         i++;
//       }
//       k = k + 1;
//       last_observed = j + k;
//     }
//   }
//   return res;
// }


class Vocabulary;
inline void process_term_vocab (const string &term,
                                Vocabulary &vocabulary,
                                unordered_map<uint32_t, uint32_t> &term_count_map);

class TermStat {
public:
  TermStat(uint32_t term_id):
  term_id(term_id), term_global_count(1), document_term_count(0) {
  };
  uint32_t term_id;
  // term count in all corpus
  uint32_t term_global_count;
  // number of documents, which contain this term
  double document_term_count;
  // share of documents, which contain this term
  //double document_share;
};

inline void term_handler (const string &term,
                          Vocabulary &vocab);

class Vocabulary {
public:
  Vocabulary(uint32_t ngram_min,
             uint32_t ngram_max,
             const string ngram_delim = "_"):
  ngram_min(ngram_min), ngram_max(ngram_max),
  token_count(0), sentence_count(0),
  ngram_delim(ngram_delim) {};

  void insert_terms (vector< string> &terms) {
    typename unordered_map < string, TermStat > :: iterator term_iterator;
    int term_id;
    for (auto it:terms) {
      this->temp_document_word_set.insert(it);
      term_iterator = this->full_vocab.find(it);
      if(term_iterator == this->full_vocab.end()) {
        term_id = this->full_vocab.size();
        // insert term into dictionary
        this->full_vocab.insert(make_pair(it, TermStat( this->full_vocab.size()) ));
      }
      else {
        term_iterator->second.term_global_count++;
      }
      this->token_count++;
    }
  }

  void insert_sentence(const CharacterVector terms) {
    this->sentence_count++;
    this->temp_document_word_set.clear();
    vector< string> ngrams = get_ngrams(terms, ngram_min, ngram_max, ngram_delim);
    insert_terms(ngrams);

    typename unordered_map < string, TermStat > :: iterator term_iterator;
    for ( auto it: this->temp_document_word_set) {
      term_iterator = full_vocab.find(it);
      if(term_iterator != full_vocab.end())
        term_iterator->second.document_term_count++;
    }
  }

  void insert_sentence_batch(const ListOf<const CharacterVector> sentence_batch) {
   for(auto s:sentence_batch)
     insert_sentence(s);
  }

  List vocab_stat() {
    size_t N = full_vocab.size();
    size_t i = 0;
    CharacterVector terms(N);
    IntegerVector term_ids(N);
    IntegerVector term_counts(N);
    IntegerVector doc_counts(N);

    for(auto it:full_vocab) {
      terms[i] = it.first;
      term_ids[i] = it.second.term_id;
      term_counts[i] = it.second.term_global_count;
      doc_counts[i] = it.second.document_term_count;
      i++;
    }
    return List::create(_["term"] = terms,
                        _["term_id"] = term_ids,
                        _["term_count"] = term_counts,
                        _["doc_count"] = doc_counts);
  }
  void increase_token_count() {token_count++;};

private:
  unordered_map< string, TermStat > full_vocab;

  uint32_t ngram_min;
  uint32_t ngram_max;
  const string ngram_delim;

  uint32_t sentence_count;
  uint32_t token_count;
  unordered_set< string > temp_document_word_set;
};
