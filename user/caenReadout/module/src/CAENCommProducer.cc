#include "eudaq/Producer.hh"
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <ratio>
#include <thread>
#ifndef _WIN32
#include <sys/file.h>
#endif

#include <CAENDigitizer.h>

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class CAENCommProducer : public eudaq::Producer {
public:
  CAENCommProducer(const std::string &name, const std::string &runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("CAENCommProducer");

private:
  bool m_flag_ts{false};
  bool m_flag_tg{false};
  uint32_t m_plane_id{0};
  FILE *m_file_lock{nullptr};
  std::chrono::milliseconds m_ms_busy;
  bool m_exit_of_run{false};

  int m_handle{0};
  CAEN_DGTZ_ReadMode_t m_mode;
  char *m_buffer{nullptr};
  uint32_t m_buffer_size{0};
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace {
  auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<
      CAENCommProducer, const std::string &, const std::string &>(
      CAENCommProducer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
CAENCommProducer::CAENCommProducer(const std::string &name,
                                   const std::string &runcontrol)
    : eudaq::Producer(name, runcontrol) {}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void CAENCommProducer::DoInitialise() {
  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("EX0_DEV_LOCK_PATH", "ex0lockfile.txt");
  m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if (flock(fileno(m_file_lock), LOCK_EX | LOCK_NB)) { // fail
    EUDAQ_THROW("unable to lock the lockfile: " + lock_path);
  }
#endif
  if (auto res = CAEN_DGTZ_OpenDigitizer(
          CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB, 0, 0, 0, &m_handle);
      res != CAEN_DGTZ_Success) {
    EUDAQ_THROW("Failed to initialise communication with the digitiser. "
                "Returned error code " +
                std::to_string(res));
  }
  CAEN_DGTZ_BoardInfo_t board_info;
  if (auto res = CAEN_DGTZ_GetInfo(m_handle, &board_info);
      res != CAEN_DGTZ_Success) {
    EUDAQ_THROW("Failed to retrieve the digitiser board info. "
                "Returned error code " +
                std::to_string(res));
  }
  EUDAQ_INFO(
      std::string("Retrieved digitiser board info:") +
      "\n* Model name: " + std::string(board_info.ModelName) +
      " (model: " + std::to_string(board_info.Model) + ")" +
      "\n* Channels: " + std::to_string(board_info.Channels) +
      ", form factor: " + std::to_string(board_info.FormFactor) +
      ", family code: " + std::to_string(board_info.FamilyCode) +
      "\n* ROC FW version: " + std::string(board_info.ROC_FirmwareRel) +
      ", AMC FW version: " + std::string(board_info.AMC_FirmwareRel) +
      "\n* Serial number: " + std::to_string(board_info.SerialNumber) +
      ", mezzanine S/N: " + std::string(board_info.MezzanineSerNum[0]) + "/" +
      std::string(board_info.MezzanineSerNum[1]) + "/" +
      std::string(board_info.MezzanineSerNum[2]) + "/" +
      std::string(board_info.MezzanineSerNum[3]) + "/" +
      std::string(board_info.MezzanineSerNum[4]) + "/" +
      std::string(board_info.MezzanineSerNum[5]) + "/" +
      std::string(board_info.MezzanineSerNum[6]) + "/" +
      std::string(board_info.MezzanineSerNum[7]) +
      "\n* PCB revision: " + std::to_string(board_info.PCB_Revision) +
      "\n* Number of ADC bits: " + std::to_string(board_info.ADC_NBits) +
      "\n* data loaded for SAM correction: " +
      std::to_string(board_info.SAMCorrectionDataLoaded) +
      "\n* Communication handle: " + std::to_string(board_info.CommHandle) +
      ", VME handle: " + std::to_string(board_info.VMEHandle));
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void CAENCommProducer::DoConfigure() {
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_plane_id = conf->Get("EX0_PLANE_ID", 0);
  m_ms_busy =
      std::chrono::milliseconds(conf->Get("EX0_DURATION_BUSY_MS", 1000));
  m_flag_ts = conf->Get("EX0_ENABLE_TIMESTAMP", 0);
  m_flag_tg = conf->Get("EX0_ENABLE_TRIGERNUMBER", 0);
  m_mode =
      static_cast<CAEN_DGTZ_ReadMode_t>(conf->Get("CAENCOMM_READOUT_MODE", 0));
  if (!m_flag_ts && !m_flag_tg) {
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp "
               "is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }

  if (auto res =
          CAEN_DGTZ_MallocReadoutBuffer(m_handle, &m_buffer, &m_buffer_size);
      res != CAEN_DGTZ_Success) {
    EUDAQ_THROW("Failed to allocate a readout buffer with the digitiser. "
                "Returned error code " +
                std::to_string(res));
  }
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void CAENCommProducer::DoStartRun() {
  if (auto res = CAEN_DGTZ_SWStartAcquisition(m_handle);
      res != CAEN_DGTZ_Success) {
    EUDAQ_THROW("Failed to start the acquisition with the digitiser. "
                "Returned error code " +
                std::to_string(res));
  }
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void CAENCommProducer::DoStopRun() {
  if (auto res = CAEN_DGTZ_SWStopAcquisition(m_handle);
      res != CAEN_DGTZ_Success) {
    EUDAQ_THROW("Failed to stop the acquisition with the digitiser. "
                "Returned error code " +
                std::to_string(res));
  }
  m_exit_of_run = true;
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void CAENCommProducer::DoReset() {
  m_exit_of_run = true;
  if (m_file_lock) {
#ifndef _WIN32
    flock(fileno(m_file_lock), LOCK_UN);
#endif
    fclose(m_file_lock);
    m_file_lock = 0;
  }
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void CAENCommProducer::DoTerminate() {
  m_exit_of_run = true;
  if (m_file_lock) {
    fclose(m_file_lock);
    m_file_lock = 0;
  }
  if (auto res = CAEN_DGTZ_CloseDigitizer(m_handle); res != CAEN_DGTZ_Success) {
    EUDAQ_THROW("Failed to close the communication with the digitiser. "
                "Returned error code " +
                std::to_string(res));
  }
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void CAENCommProducer::RunLoop() {
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  uint8_t x_pixel = 16;
  uint8_t y_pixel = 16;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> position(0, x_pixel * y_pixel - 1);
  std::uniform_int_distribution<uint32_t> signal(0, 255);
  while (!m_exit_of_run) {
    auto ev = eudaq::Event::MakeUnique("CAENCommRaw");
    ev->SetTag("Plane ID", std::to_string(m_plane_id));
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    if (m_flag_ts) {
      std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
      std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
      ev->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());
    }
    if (m_flag_tg)
      ev->SetTriggerN(trigger_n);

    uint32_t buffer_size;
    if (auto res = CAEN_DGTZ_ReadData(m_handle, m_mode, m_buffer, &buffer_size);
        res != CAEN_DGTZ_Success) {
      EUDAQ_THROW("Failed to stop the acquisition with the digitiser. "
                  "Returned error code " +
                  std::to_string(res));
    }

    std::vector<uint8_t> hit(x_pixel * y_pixel, 0);
    hit[position(gen)] = signal(gen);
    std::vector<uint8_t> data;
    data.push_back(x_pixel);
    data.push_back(y_pixel);
    data.insert(data.end(), hit.begin(), hit.end());

    uint32_t block_id = m_plane_id;
    ev->AddBlock(block_id, data);
    SendEvent(std::move(ev));
    trigger_n++;
    std::this_thread::sleep_until(tp_end_of_busy);
  }
}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------
