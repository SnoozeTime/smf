#pragma once
// std
#include <ostream>
// seastar
#include <core/future.hh>
#include <core/sstring.hh>
// smf
#include "filesystem/wal_opts.h"
#include "filesystem/wal_requests.h"

namespace smf {
class wal_iface {
  public:
  /// \brief returns starting offset off a successful write
  virtual future<uint64_t> append(wal_write_request req) = 0;
  virtual future<> invalidate(uint64_t epoch) = 0;
  virtual future<wal_opts::maybe_buffer> get(wal_read_request req) = 0;

  // \brief filesystem monitoring
  virtual future<> open() = 0;
  virtual future<> close() = 0;
};

/// brief - write ahead log
class wal : public wal_iface {
  public:
  explicit wal(wal_opts o) : opts(std::move(o)) {}
  virtual ~wal() {}
  /// brief - used by seastar map-reduce
  inline const wal_opts &get_otps() const { return opts; }


  static std::unique_ptr<wal> make_wal(wal_type type, wal_opts opts);

  protected:
  wal_opts opts;
};

// simple wrapper for seastar constructs
class shardable_wal : public wal_iface {
  public:
  shardable_wal(wal_type type, wal_opts o)
    : w_(wal::make_wal(type, std::move(o))) {}
  inline virtual future<uint64_t> append(wal_write_request req) override final {
    return w_->append(std::move(req));
  }
  inline virtual future<> invalidate(uint64_t epoch) override final {
    return w_->invalidate(epoch);
  }
  inline virtual future<wal_opts::maybe_buffer>
  get(wal_read_request req) override final {
    return w_->get(std::move(req));
  }
  inline virtual future<> open() override final { return w_->open(); }
  inline virtual future<> close() override final { return w_->close(); }
  // seastar shardable template param
  future<> stop() { return close(); }

  private:
  std::unique_ptr<wal> w_;
};
} // namespace smf
