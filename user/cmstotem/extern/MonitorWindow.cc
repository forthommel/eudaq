#include "MonitorWindow.hh"

#include "TApplication.h"
#include "TTimer.h"
#include "TGButton.h"
#include "TFile.h"
#include "TGFileDialog.h"
#include "TCanvas.h"

// required for object-specific "clear"
#include "TGraph.h"
#include "TH1.h"
#include "TMultiGraph.h"

#include <iostream>
#include <fstream>
#include <sstream>

MonitorWindow::MonitorWindow(TApplication* par, const std::string& name)
  :TGMainFrame(gClient->GetRoot(), 800, 600, kVerticalFrame), m_parent(par),
   m_icon_save(gClient->GetPicture("bld_save.xpm")),
   m_icon_del(gClient->GetPicture("bld_delete.xpm")),
   m_icon_th1(gClient->GetPicture("h1_t.xpm")),
   m_icon_th2(gClient->GetPicture("h2_t.xpm")),
   m_icon_tgraph(gClient->GetPicture("graph.xpm")),
   m_icon_track(gClient->GetPicture("eve_track.xpm")),
   m_icon_summ(gClient->GetPicture("draw_t.xpm")),
   m_timer(new TTimer(1000, kTRUE)){
  SetWindowName(name.c_str());

  m_top_win = new TGHorizontalFrame(this);
  m_left_bar = new TGVerticalFrame(m_top_win);
  m_left_canv = new TGCanvas(m_left_bar, 200, 600);

  auto vp = m_left_canv->GetViewPort();

  m_tree_list = new TGListTree(m_left_canv, kHorizontalFrame);
  m_tree_list->Connect("DoubleClicked(TGListTreeItem*, Int_t)", NAME, this,
                       "DrawElement(TGListTreeItem*, Int_t)");
  m_tree_list->Connect("Clicked(TGListTreeItem*, Int_t, Int_t, Int_t)",
                       NAME, this,
                       "DrawMenu(TGListTreeItem*, Int_t, Int_t, Int_t)");
  vp->AddFrame(m_tree_list, new TGLayoutHints(kLHintsExpandY | kLHintsExpandY, 5, 5, 5, 5));
  m_top_win->AddFrame(m_left_bar, new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
  m_left_bar->AddFrame(m_left_canv, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));

  auto right_frame = new TGVerticalFrame(m_top_win);

  // toolbar
  m_toolbar = new TGToolBar(right_frame, 180, 80);
  right_frame->AddFrame(m_toolbar, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 2));
  m_button_save = new TGPictureButton(m_toolbar, m_icon_save);
  m_button_save->SetToolTipText("Save all monitors");
  m_button_save->SetEnabled(false);
  m_button_save->Connect("Clicked()", NAME, this, "SaveFile()");
  m_toolbar->AddFrame(m_button_save, new TGLayoutHints(kLHintsLeft, 2, 1, 0, 0));

  m_button_clean = new TGPictureButton(m_toolbar, m_icon_del);
  m_button_clean->SetToolTipText("Clean all monitors");
  m_button_clean->SetEnabled(false);
  m_button_clean->Connect("Clicked()", NAME, this, "ClearMonitors()");
  m_toolbar->AddFrame(m_button_clean, new TGLayoutHints(kLHintsLeft, 1, 2, 0, 0));

  auto update_toggle = new TGCheckButton(m_toolbar, "&Update", 1);
  m_toolbar->AddFrame(update_toggle, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));
  update_toggle->SetToolTipText("Switch on/off the auto refresh of all monitors");
  update_toggle->Connect("Toggled(Bool_t)", NAME, this, "SwitchUpdate(Bool_t)");

  // main canvas
  m_main_canvas = new TRootEmbeddedCanvas("Canvas", right_frame);
  right_frame->AddFrame(m_main_canvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
  m_top_win->AddFrame(right_frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
  this->AddFrame(m_top_win, new TGLayoutHints(kLHintsExpandY | kLHintsExpandX | kLHintsLeft, 0, 0, 0, 0));

  // status bar
  int status_parts[(int)StatusBarPos::num_parts] = {20, 10, 35, 35};
  m_status_bar = new TGStatusBar(this, 510, 10, kHorizontalFrame);
  m_status_bar->SetParts(status_parts, (int)StatusBarPos::num_parts);
  ResetCounters();

  this->AddFrame(m_status_bar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));
  //left_bar->MapSubwindows();

  m_timer->Connect("Timeout()", NAME, this, "Update()");
  update_toggle->SetOn();
  SwitchUpdate(true);

  this->MapSubwindows();
  this->Layout();
  this->MapSubwindows();
  this->MapWindow();
  m_context_menu = new TContextMenu("", "");
}

MonitorWindow::~MonitorWindow(){
  // unregister the icons
  gClient->FreePicture(m_icon_save);
  gClient->FreePicture(m_icon_del);
  gClient->FreePicture(m_icon_th1);
  gClient->FreePicture(m_icon_th2);
  gClient->FreePicture(m_icon_tgraph);
  gClient->FreePicture(m_icon_track);
  if (m_parent)
    m_parent->Terminate(1);
}

void MonitorWindow::SetCounters(unsigned long long evt_recv, unsigned long long evt_mon){
  m_last_event = evt_recv;
  m_last_event_mon = evt_mon;
}

void MonitorWindow::ResetCounters(){
  if (m_status_bar) {
    m_status_bar->SetText("Run: N/A", (int)StatusBarPos::run_number);
    m_status_bar->SetText("Curr. event: N/A", (int)StatusBarPos::tot_events);
    m_status_bar->SetText("Analysed events: N/A", (int)StatusBarPos::an_events);
  }
  m_button_save->SetEnabled(false);
  m_button_clean->SetEnabled(false);
  ClearMonitors();
}

void MonitorWindow::SetRunNumber(int run){
  m_run_number = run;
  if (m_status_bar)
    m_status_bar->SetText(Form("Run: %u", m_run_number), (int)StatusBarPos::run_number);
  m_button_save->SetEnabled(true);
  m_button_clean->SetEnabled(true);
}

void MonitorWindow::SetLastEventNum(int num){
  if (m_status_bar && m_status == Status::running)
    m_status_bar->SetText(Form("Curr. event: %d", num), (int)StatusBarPos::tot_events);
}

void MonitorWindow::SetMonitoredEventsNum(int num){
  if (m_status_bar && m_status == Status::running)
    m_status_bar->SetText(Form("Analysed events: %d", num), (int)StatusBarPos::an_events);
}

void MonitorWindow::SetStatus(Status st){
  m_status = st;
  if (m_status_bar) {
    std::ostringstream st_txt; st_txt << st;
    m_status_bar->SetText(st_txt.str().c_str(), (int)StatusBarPos::status);
  }
}

void MonitorWindow::SaveFile(){
  TGFileInfo fi;
  { // first define the output file
    static TString dir(".");
    const char *filetypes[] = {"ROOT files", "*.root",
                               0,            0 };
    fi.fFileTypes = filetypes;
    fi.fIniDir = StrDup(dir);
    new TGFileDialog(gClient->GetRoot(), this, kFDSave, &fi);
    dir = fi.fIniDir;
  }
  // then save all collections
  auto file = std::unique_ptr<TFile>(TFile::Open(fi.fFilename, "recreate"));
  for (auto& obj : m_objects) {
    TString s_file(obj.first);
    auto pos = s_file.Last('/');
    TString s_path;
    if (pos != kNPOS) { // substructure found
      s_path = s_file;
      s_path.Remove(pos);
      s_file.Remove(0, pos+1);
      if (!file->GetDirectory(s_path))
        file->mkdir(s_path);
    }
    file->cd(s_path);
    if (obj.second.object->Write(s_file) == 0)
      std::cerr << "[WARNING] Failed to write \"" << obj.first << "\" into the output file!" << std::endl;
  }
  file->Close();
}

void MonitorWindow::SwitchUpdate(bool up){
  if (!up)
    m_timer->Stop();
  else if (up)
    m_timer->Start(-1, kFALSE); // update automatically
}

void MonitorWindow::Update(){
  SetLastEventNum(m_last_event);
  SetMonitoredEventsNum(m_last_event_mon);

  if (m_drawable.empty()) // nothing to draw
    return;
  TCanvas* canv = m_main_canvas->GetCanvas();
  if (!canv) { // failed to retrieve the plotting region
    std::cerr << "[WARNING] Failed to retrieve the main plotting canvas!" << std::endl;
    return;
  }
  canv->cd();
  canv->Clear();
  if (m_drawable.size() > 1) {
    int ncol = ceil(sqrt(m_drawable.size()));
    int nrow = ceil(m_drawable.size()*1./ncol);
    canv->Divide(ncol, nrow);
  }
  for (size_t i = 0; i < m_drawable.size(); ++i) {
    canv->cd(i+1);
    auto& dr = m_drawable.at(i);
    dr->object->Draw(dr->draw_opt);
    if (dr->object->InheritsFrom("TH1")
	&& dr->min_y != kInvalidValue && dr->max_y != kInvalidValue)
      dynamic_cast<TH1*>(dr->object)->GetYaxis()->SetRangeUser(dr->min_y, dr->max_y);
    else if (dr->object->InheritsFrom("TGraph")
	&& dr->min_y != kInvalidValue && dr->max_y != kInvalidValue)
      dynamic_cast<TGraph*>(dr->object)->GetYaxis()->SetRangeUser(dr->min_y, dr->max_y);
  }
  canv->Update();
  // clean all non-persistent objects
  for (auto& dr : m_drawable)
    if (!dr->persist)
      CleanObject(dr->object);
}

TObject* MonitorWindow::Get(const std::string& name){
  auto it = m_objects.find(name);
  if (it == m_objects.end())
    throw std::runtime_error("Failed to retrieve object with path \""+std::string(name)+"\"!");
  return it->second.object;
}

void MonitorWindow::DrawElement(TGListTreeItem* it, int val){
  m_drawable.clear();
  for (auto& obj : m_objects)
    if (obj.second.item == it)
      m_drawable.emplace_back(&obj.second);
  if (m_drawable.empty()) // did not find in objects, must be a directory
    for (auto& obj : m_objects)
      if (obj.second.item->GetParent() == it)
        m_drawable.emplace_back(&obj.second);
  if (m_drawable.empty()) // did not find in directories either, must be a summary
    if (m_summ_objects.count(it) > 0)
      for (auto& obj : m_summ_objects[it])
	m_drawable.emplace_back(obj);
  Update();
}

void MonitorWindow::DrawMenu(TGListTreeItem* it, int but, int x, int y){
  if (but == 3)
    m_context_menu->Popup(x, y, this);
}

void MonitorWindow::SetPersistant(const TObject* obj, bool pers){
  for (auto& o : m_objects)
    if (o.second.object == obj) {
      o.second.persist = pers;
      return;
    }
}

void MonitorWindow::SetDrawOptions(const TObject* obj, Option_t* opt){
  for (auto& o : m_objects)
    if (o.second.object == obj) {
      o.second.draw_opt = opt;
      return;
    }
}

void MonitorWindow::SetRangeY(const TObject* obj, double min, double max){
  for (auto& o : m_objects)
    if (o.second.object == obj) {
      o.second.min_y = min;
      o.second.max_y = max;
      return;
    }
}

TGListTreeItem* MonitorWindow::BookStructure(const std::string& path, TGListTreeItem* par){
  auto tok = TString(path).Tokenize("/");
  if (tok->IsEmpty())
    return par;
  TGListTreeItem* prev = nullptr;
  std::string full_path;
  for (int i = 0; i < tok->GetEntriesFast()-1; ++i) {
    const auto iter = tok->At(i);
    TString dir_name = dynamic_cast<TObjString*>(iter)->String();
    full_path += dir_name+"/";
    if (m_dirs.count(full_path) == 0)
      m_dirs[full_path] = m_tree_list->AddItem(prev, dir_name);
    prev = m_dirs[full_path];
  }
  return prev;
}

void MonitorWindow::AddSummary(const std::string& path, const TObject* obj){
  for (auto& o : m_objects) {
    if (o.second.object != obj)
      continue;
    if (m_dirs.count(path) == 0) {
      auto obj_name = path;
      obj_name.erase(0, obj_name.rfind('/')+1);
      m_dirs[path] = m_tree_list->AddItem(BookStructure(path), obj_name.c_str());
      m_dirs[path]->SetPictures(m_icon_summ, m_icon_summ);
    }
    m_summ_objects[m_dirs[path]].emplace_back(&o.second);
    m_left_canv->MapSubwindows();
    m_left_canv->MapWindow();
    return;
  }
  throw std::runtime_error("Failed to retrieve an object for summary \""+path+"\"");
}

void MonitorWindow::ClearMonitors(){
  std::cout << "Clearing of all monitors requested" << std::endl;
  // last loop to clear non-persistent objects before next refresh
  for (auto& dr : m_objects)
    CleanObject(dr.second.object);
}

void MonitorWindow::CleanObject(TObject* obj){
  if (obj->InheritsFrom("TMultiGraph")) { // special case for this one
    for (auto gr : *dynamic_cast<TMultiGraph*>(obj))
      delete gr;
  }
  else if (obj->InheritsFrom("TGraph"))
    dynamic_cast<TGraph*>(obj)->Set(0);
  else if (obj->InheritsFrom("TH1"))
    dynamic_cast<TH1*>(obj)->Reset();
  else
    std::cerr
      << "[WARNING] monitoring object with class name "
      << "\"" << obj->ClassName() << "\" cannot be cleared"
      << std::endl;
}

std::ostream& operator<<(std::ostream& os, const MonitorWindow::Status& stat){
  switch (stat) {
    case MonitorWindow::Status::idle: return os << "IDLE";
    case MonitorWindow::Status::configured: return os << "CONFIGURED";
    case MonitorWindow::Status::running: return os << "RUNNING";
    case MonitorWindow::Status::error: return os << "ERROR";
  }
  return os << "UNKNOWN";
}