#ifndef JIL_SPLITTER_H_
#define JIL_SPLITTER_H_
#pragma once

#include <string>
#include "wx/panel.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////

class SplitLeaf;

class SplitNode {
 public:
  SplitNode(wxOrientation orientation = wxVERTICAL, double size_ratio = 0.5f)
      : orientation_(orientation)
      , size_ratio_(size_ratio)
      , node1_(NULL)
      , node2_(NULL) {
  }

  virtual ~SplitNode();

  virtual SplitLeaf* AsLeaf() {
    return NULL;
  }

  virtual bool IsShown() const;

  wxOrientation orientation() const {
    return orientation_;
  }

  void set_orientation(wxOrientation orientation) {
    orientation_ = orientation;
  }

  bool vertical() const {
    return orientation_ == wxVERTICAL;
  }

  double size_ratio() const {
    return size_ratio_;
  }

  void set_size_ratio(double size_ratio) {
    size_ratio_ = size_ratio;
  }

  const wxRect& rect() const {
    return rect_;
  }

  void set_rect(const wxRect& rect) {
    rect_ = rect;
  }

  SplitNode* node1() const {
    return node1_;
  }

  void set_node1(SplitNode* node1) {
    node1_ = node1;
  }

  SplitNode* node2() const {
    return node2_;
  }

  void set_node2(SplitNode* node2) {
    node2_ = node2;
  }

  virtual wxSize GetMinSize() const;

 protected:
  // wxHORIZONTAL or wxVERTICAL.
  wxOrientation orientation_;

  // 0.0 - 1.0
  double size_ratio_;

  wxRect rect_;

  SplitNode* node1_;
  SplitNode* node2_;
};

////////////////////////////////////////////////////////////////////////////////

class SplitLeaf : public SplitNode {
 public:
  explicit SplitLeaf(wxWindow* window)
      : window_(window) {
  }

  virtual ~SplitLeaf() {
  }

  virtual SplitLeaf* AsLeaf() override {
    return this;
  }

  virtual bool IsShown() const override {
    return (window_ != NULL && window_->IsShown());
  }

  wxWindow* window() const {
    return window_;
  }

  void set_window(wxWindow* window) {
    window_ = window;
  }

  const std::string& id() const {
    return id_;
  }

  void set_id(const std::string& id) {
    id_ = id;
  }

  virtual wxSize GetMinSize() const override;

 private:
  wxWindow* window_;
  std::string id_;
};

////////////////////////////////////////////////////////////////////////////////

// Example:
// A, B, C are three windows. The layout is:
// +-------------------------------------------------------+
// |                          |                            |
// |                          |             B              |
// |                          |                            |
// |            A             |----------------------------+
// |                          |                            |
// |                          |             C              |
// |                          |                            |
// +-------------------------------------------------------+
// Serialization:
// split = {
//   orientation = "vertical";
//   size_ratio = 0.5;
//   node1 = {
//     id = "A";
//   };
//   node2 = {
//     orientation = "horizontal";
//     size_ratio = 0.5;
//     node1 = {
//       id = "B";
//     };
//     node2 = {
//       id = "C";
//     };
//   };
// };

class Splitter : public wxPanel {
  DECLARE_EVENT_TABLE()

 public:
  Splitter();
  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);
  virtual ~Splitter();

  // Set split tree.
  // NOTE: The user manages the life-cycle of the split tree.
  void SetSplitRoot(SplitNode* split_root);

  void Split();

 protected:
  void Split(SplitNode* n, const wxRect& rect);

  void OnSize(wxSizeEvent& evt);

  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseMove(wxMouseEvent& evt);

  // Find the split node whose gap contains the given point.
  SplitNode* GapNode(SplitNode* n, const wxPoint& p) const;
  SplitNode* GapNode(const wxPoint& p) const {
    return GapNode(split_root_, p);
  }

  void SetSizeCursor(wxOrientation orientation);

 private:
  // Just a reference.
  SplitNode* split_root_;

  wxPoint down_point_;
  wxRect down_rect1_;
  SplitNode* gap_node_;
};

}  // namespace jil

#endif  // JIL_SPLITTER_H_
