//***************************************************************************
//* Copyright (c) 2011-2013 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

#ifndef HAMMER_VALIDKMERGENERATOR_HPP_
#define HAMMER_VALIDKMERGENERATOR_HPP_

#include "globals.hpp"

#include "io/read.hpp"
#include "sequence/seq.hpp"

#include <string>
#include <vector>

#include <cstdint>
#include <cmath>

/**
 * This class is designed to iterate through valid k-mers in read.
 * @example
 *   ValidKMerGenerator<2> gen(read, 4);
 *   while (gen.HasMore()) {
 *     MyTrickyFunction(gen.kmer());
 *     gen.Next();
 *   }
 *   or
 *   for (ValidKMerGenerator<2> gen(read, 2); gen.HasMore; gen.Next() {
 *     MyTrickyFunction(gen.kmer(), gen.pos(), gen.correct_probability());
 *   }
 * @param kK k-mer length.
 */
template<uint32_t kK>
class ValidKMerGenerator {
 public:
  /**
   * @param read Read to generate k-mers from.
   * @param bad_quality_threshold  This class virtually cuts
   * nucleotides with quality lower the threshold from the ends of the
   * read.
   */
  explicit ValidKMerGenerator(const Read &read,
                              uint32_t bad_quality_threshold = 2) {
    Reset(read.getSequenceString().data(),
          read.getQualityString().data(),
          read.getSequenceString().size(),
          bad_quality_threshold);
  }
  /**
   * @param seq sequence to generate k-mers from.
   * @param qual quality string
   * @param bad_quality_threshold  This class virtually cuts
   * nucleotides with quality lower the threshold from the ends of the
   * read.
   */
  explicit ValidKMerGenerator(const char *seq, const char *qual,
                              size_t len,
                              uint32_t bad_quality_threshold = 2) {
    Reset(seq, qual, len, bad_quality_threshold);
  }

  ValidKMerGenerator()
      : kmer_(), seq_(0), qual_(0),
        pos_(-1), end_(-1), len_(0),
        correct_probability_(1), bad_quality_threshold_(2),
        has_more_(false), first(true) {}

  void Reset(const char *seq, const char *qual,
             size_t len,
             uint32_t bad_quality_threshold = 2) {
    kmer_ = Seq<kK>();
    seq_ = seq;
    qual_ = qual;
    pos_ = -1;
    end_ = -1;
    len_ = len;
    correct_probability_ = 1.0;
    bad_quality_threshold_ = bad_quality_threshold;
    has_more_ = true;
    first = true;

    TrimBadQuality();
    Next();
  }

  /**
   * @result true if Next() succeed while generating new k-mer, false
   * otherwise.
   */
  bool HasMore() const {
    return has_more_;
  }
  /**
   * @result last k-mer generated by Next().
   */
  const Seq<kK>& kmer() const {
    return kmer_;
  }
  /**
   * @result last k-mer position in initial read.
   */
  int pos() const {
    return pos_;
  }
  /**
   * @result probability that last generated k-mer is correct.
   */
  double correct_probability() const {
    return correct_probability_;
  }
  /**
   * This functions reads next k-mer from the read and sets hasmore to
   * if succeeded. You can access k-mer read with kmer().
   */
  void Next();
 private:
  void TrimBadQuality();
  double Prob(uint8_t qual) {
    return Globals::quality_probs[qual];
  }
  uint32_t GetQual(uint32_t pos) {
    if (pos >= len_) {
      return 2;
    } else {
      return qual_[pos];
    }
  }
  Seq<kK> kmer_;
  const char* seq_;
  const char* qual_;
  size_t pos_;
  size_t end_;
  size_t len_;
  double correct_probability_;
  uint32_t bad_quality_threshold_;
  bool has_more_;
  bool first;

  // Disallow copy and assign
  ValidKMerGenerator(const ValidKMerGenerator&) = delete;
  void operator=(const ValidKMerGenerator&) = delete;
};

template<uint32_t kK>
void ValidKMerGenerator<kK>::TrimBadQuality() {
  pos_ = 0;
  if (qual_)
    for (; pos_ < len_; ++pos_) {
      if (GetQual(pos_) >= bad_quality_threshold_)
        break;
    }
  end_ = len_;
  if (qual_)
    for (; end_ > pos_; --end_) {
      if (GetQual(end_ - 1) >= bad_quality_threshold_)
        break;
  }
}

template<uint32_t kK>
void ValidKMerGenerator<kK>::Next() {
  if (pos_ + kK > end_) {
    has_more_ = false;
  } else if (first || !is_nucl(seq_[pos_ + kK - 1])) {
    // in this case we have to look for new k-mer
    correct_probability_ = 1.0;
    uint32_t start_hypothesis = pos_;
    uint32_t i = pos_;
    for (; i < len_; ++i) {
      if (i == kK + start_hypothesis) {
        break;
      }
      if (qual_)
        correct_probability_ *= Prob(GetQual(i));
      if (!is_nucl(seq_[i])) {
        start_hypothesis = i + 1;
        correct_probability_ = 1.0;
      }
    }
    if (i == kK + start_hypothesis) {
      kmer_ = Seq<kK>(seq_ + start_hypothesis, 0, kK, /* raw */ true);
      pos_ = start_hypothesis + 1;
    } else {
      has_more_ = false;
    }
  } else {
    // good case we can just shift our previous answer
    kmer_ = kmer_ << seq_[pos_ + kK - 1];
    if (qual_) {
      correct_probability_ *= Prob(GetQual(pos_ + kK - 1));
      correct_probability_ /= Prob(GetQual(pos_ - 1));
    }
    ++pos_;
  }
  first = false;
}
#endif  // HAMMER_VALIDKMERGENERATOR_HPP__
