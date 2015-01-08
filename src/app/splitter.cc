#include "app/splitter.h"
#include "wx/log.h"
#include "wx/wupdlock.h"

namespace jil {

static const int kGap = 3;  // px

////////////////////////////////////////////////////////////////////////////////

SplitNode::~SplitNode() {
  if (node1_ != NULL) {
    delete node1_;
  }
  if (node2_ != NULL) {
    delete node2_;
  }
}

bool SplitNode::IsShown() const {
  if (node1_ != NULL) {
    if (!node1_->IsShown()) {
      return false;
    }
  }

  if (node2_ != NULL) {
    if (!node2_->IsShown()) {
      return false;
    }
  }

  return true;
}

wxSize SplitNode::GetMinSize() const {
  wxSize min_size;

  wxSize min_size1 = node1_ != NULL ? node1_->GetMinSize() : wxDefaultSize;
  wxSize min_size2 = node2_ != NULL ? node2_->GetMinSize() : wxDefaultSize;

  if (vertical()) {
  } else {
    min_size.x = wxMax(min_size1.x, min_size2.x);

    if (min_size1.y != -1) {
      min_size.y += min_size1.y;
    }
    if (min_size2.y != -1) {
      min_size.y += min_size2.y;
    }
    if (min_size.y == 0) {
      min_size.y = -1;
    }
  }

  return min_size;
}

////////////////////////////////////////////////////////////////////////////////

wxSize SplitLeaf::GetMinSize() const {
  if (window_ != NULL) {
    return window_->GetMinSize();
  }
  return wxDefaultSize;
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(Splitter, wxPanel)
EVT_SIZE      (Splitter::OnSize)
EVT_LEFT_DOWN (Splitter::OnMouseLeftDown)
EVT_LEFT_UP   (Splitter::OnMouseLeftUp)
EVT_MOTION    (Splitter::OnMouseMove)
END_EVENT_TABLE()

Splitter::Splitter()
    : split_root_(NULL)
    , gap_node_(NULL) {
}

bool Splitter::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  return true;
}

Splitter::~Splitter() {
}

void Splitter::SetSplitRoot(SplitNode* split_root) {
  if (split_root_ == split_root) {
    return;
  }
  if (split_root_ != NULL) {
    delete split_root_;
  }
  split_root_ = split_root;
}

void Splitter::Split() {
  if (split_root_ != NULL) {
    wxWindowUpdateLocker no_update(this);
    Split(split_root_, GetClientRect());
  }
}

void Splitter::Split(SplitNode* n, const wxRect& rect) {
  n->set_rect(rect);

  SplitLeaf* leaf = n->AsLeaf();
  if (leaf != NULL) {
    leaf->window()->SetSize(rect);
    return;
  }

  if (n->node1() == NULL || n->node2() == NULL) {
    wxLogWarning(wxT("Non-leaf split node must have 2 child nodes!"));
    return;
  }

  wxRect rect1;
  wxRect rect2;

  if (n->vertical()) {
    rect1.x = rect.x;
    rect1.y = rect.y;
    rect1.width = static_cast<int>(rect.width * n->size_ratio());
    rect1.height = rect.height;

    rect2.x = rect.x + rect1.width + kGap;
    rect2.y = rect.y;
    rect2.width = rect.width - rect1.width - kGap;
    rect2.height = rect.height;

    if (!n->node1()->IsShown()) {
      rect2.width = rect.width;
    }
    if (!n->node2()->IsShown()) {
      rect1.width = rect.width;
    }
  } else {
    rect1.x = rect.x;
    rect1.y = rect.y;
    rect1.width = rect.width;
    rect1.height = static_cast<int>(rect.height * n->size_ratio());

    rect2.x = rect.x;
    rect2.y = rect.y + rect1.height + kGap;
    rect2.width = rect.width;
    rect2.height = rect.height - rect1.height - kGap;

    if (!n->node1()->IsShown()) {
      rect2.height = rect.height;
    }
    if (!n->node2()->IsShown()) {
      rect1.height = rect.height;
    }
  }

  if (n->node1()->IsShown()) {
    Split(n->node1(), rect1);
  }
  if (n->node2()->IsShown()) {
    Split(n->node2(), rect2);
  }
}

void Splitter::OnSize(wxSizeEvent& evt) {
  Split();
  evt.Skip();
}

void Splitter::OnMouseLeftDown(wxMouseEvent& evt) {
  gap_node_ = GapNode(evt.GetPosition());
  if (gap_node_ == NULL) {
    // Not down inside the gap, no resize.
    return;
  }

  if (!HasCapture()) {
    CaptureMouse();
  }

  SetSizeCursor(gap_node_->orientation());

  down_point_ = evt.GetPosition();
  down_rect1_ = gap_node_->node1()->rect();
}

void Splitter::OnMouseLeftUp(wxMouseEvent& evt) {
  if (!HasCapture()) {
    return;
  }
  gap_node_ = NULL;
  ReleaseMouse();
}

void Splitter::OnMouseMove(wxMouseEvent& evt) {
  if (gap_node_ == NULL) {
    SplitNode* gap_node = GapNode(evt.GetPosition());
    if (gap_node != NULL) {
      SetSizeCursor(gap_node->orientation());
    }
    return;
  }

  // Set cursor.
  SetSizeCursor(gap_node_->orientation());

  // Check resize condition.
  if (!HasCapture() || !evt.Dragging() || !evt.LeftIsDown()) {
    return;
  }

  // Calculate new size ratio.
  double new_size_ratio = 0.0f;

  if (gap_node_->vertical()) {
    int delta = evt.GetPosition().x - down_point_.x;
    if (delta == 0) {
      return;
    }

    int width = gap_node_->rect().width;

    int width1 = down_rect1_.width + delta;
    if (width1 < 0) {
      width1 = 0;
    } else if (width1 > width) {
      width1 = width - kGap;
    }

    // Mind the min width of node1.
    int min_width1 = gap_node_->node1()->GetMinSize().x;
    if (width1 < min_width1) {
      width1 = min_width1;
    } else {
      // Mind the min width of node2.
      int min_width2 = gap_node_->node2()->GetMinSize().x;
      int width2 = width - width1 - kGap;
      if (width2 < min_width2) {
        width2 = min_width2;
        width1 = width - width2 - kGap;
      }
    }

    new_size_ratio = static_cast<double>(width1) / width;

  } else {
    int delta = evt.GetPosition().y - down_point_.y;
    if (delta == 0) {
      return;
    }

    int height = gap_node_->rect().height;

    int height1 = down_rect1_.height + delta;
    if (height1 < 0) {
      height1 = 0;
    } else if (height1 > height) {
      height1 = height - kGap;
    }

    // Mind the min height of node1.
    int min_height1 = gap_node_->node1()->GetMinSize().y;
    if (height1 < min_height1) {
      height1 = min_height1;
    } else {
      // Mind the min height of node2.
      int min_height2 = gap_node_->node2()->GetMinSize().y;
      int height2 = height - height1 - kGap;
      if (height2 < min_height2) {
        height2 = min_height2;
        height1 = height - height2 - kGap;
      }
    }

    new_size_ratio = static_cast<double>(height1) / height;
  }

  // Update size ratio.
  gap_node_->set_size_ratio(new_size_ratio);

  // Update layout.
  Split();
}

SplitNode* Splitter::GapNode(SplitNode* n, const wxPoint& p) const {
  if (n->AsLeaf() != NULL) {
    return NULL;
  }

  if (n->node1() == NULL || n->node2() == NULL) {
    wxLogWarning(wxT("Non-leaf split node must have 2 child nodes!"));
    return NULL;
  }

  if (!n->node1()->IsShown() || !n->node2()->IsShown()) {
    return NULL;
  }

  wxRect gap_rect;
  wxRect rect1 = n->node1()->rect();
  if (n->vertical()) {
    gap_rect = wxRect(rect1.GetRight(), rect1.y, kGap, rect1.height);
  } else {
    gap_rect = wxRect(rect1.x, rect1.GetBottom(), rect1.width, kGap);
  }

  if (gap_rect.Contains(p)) {
    return n;
  }

  if (n->node1()->rect().Contains(p)) {
    return GapNode(n->node1(), p);
  } else if (n->node2()->rect().Contains(p)) {
    return GapNode(n->node2(), p);
  }

  return NULL;
}

void Splitter::SetSizeCursor(wxOrientation orientation) {
  wxStockCursor cursor = wxCURSOR_NONE;
  if (orientation == wxVERTICAL) {
    cursor = wxCURSOR_SIZEWE;
  } else {
    cursor = wxCURSOR_SIZENS;
  }
  wxSetCursor(wxCursor(cursor));
}

}  // namespace jil
