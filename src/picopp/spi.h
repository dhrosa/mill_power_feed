#pragma once

#include <hardware/spi.h>

#include <span>
#include <utility>

// TODO(dhrosa): Referring to
// https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf page 519 , we
// should experiment try 16-bit writes, as 8-bit writes waste half of the FIFO
// space.
//
// C++ wrapper around hardware_spi.
class Spi {
 public:
  Spi(spi_inst_t* spi, unsigned baudrate);

  ~Spi();

  void operator=(const Spi&) = delete;

  spi_inst_t* get() const { return spi_; }

  int Write(std::span<const std::uint8_t> payload);

 private:
  spi_inst_t* spi_ = nullptr;
};

inline Spi::Spi(spi_inst_t* spi, unsigned baudrate) : spi_(spi) {
  spi_init(spi_, baudrate);
}

inline Spi::~Spi() {
  if (spi_) spi_deinit(spi_);
}

inline int Spi::Write(std::span<const std::uint8_t> payload) {
  return spi_write_blocking(get(), payload.data(), payload.size());
}
