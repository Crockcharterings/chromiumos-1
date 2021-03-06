// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINDOW_MANAGER_WINDOW_MANAGER_H_
#define WINDOW_MANAGER_WINDOW_MANAGER_H_

#include <map>
#include <set>
#include <vector>
#include <tr1/memory>
#include <utility>

extern "C" {
// TODO: Move the event-handling methods (all private) into a separate
// header so that these includes can be removed.
#include <X11/extensions/shape.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>
}

#include <gtest/gtest_prod.h>  // for FRIEND_TEST() macro

#include "base/scoped_ptr.h"
#include "window_manager/atom_cache.h"  // for Atom enum
#include "window_manager/clutter_interface.h"
#include "window_manager/compositor_event_source.h"
#include "window_manager/wm_ipc.h"
#include "window_manager/x_types.h"

namespace window_manager {

class EventConsumer;
class HotkeyOverlay;
class KeyBindings;
class LayoutManager;
class MetricsReporter;
class PanelManager;
class StackingManager;
class Window;
class WmIpc;
class XConnection;
template<class T> class Stacker;

class WindowManager : public CompositorEventSource {
 public:
  WindowManager(XConnection* xconn, ClutterInterface* clutter);
  ~WindowManager();

  XConnection* xconn() { return xconn_; }
  ClutterInterface* clutter() { return clutter_; }
  StackingManager* stacking_manager() { return stacking_manager_.get(); }

  XWindow root() const { return root_; }
  XWindow background_xid() const { return background_xid_; }

  ClutterInterface::StageActor* stage() { return stage_; }
  ClutterInterface::Actor* background() { return background_.get(); }

  int width() const { return width_; }
  int height() const { return height_; }

  XWindow wm_xid() const { return wm_xid_; }
  XWindow active_window_xid() const { return active_window_xid_; }

  KeyBindings* key_bindings() { return key_bindings_.get(); }
  WmIpc* wm_ipc() { return wm_ipc_.get(); }
  int wm_ipc_version() const { return wm_ipc_version_; }

  // Begin CompositorEventSource implementation.
  void StartSendingEventsForWindowToCompositor(XWindow xid);
  void StopSendingEventsForWindowToCompositor(XWindow xid);
  // End CompositorEventSource implementation.

  // Get the title for the window that we create to take ownership of management
  // selections.  This is also used to name our log files.
  static const char* GetWmName() { return "chromeos-wm"; }

  // Perform initial setup.  This must be called immediately after the
  // WindowManager object is created.
  bool Init();

  // Handle an event from the X server.
  void HandleEvent(XEvent* event);

  // Create a new X window for receiving input.
  XWindow CreateInputWindow(
      int x, int y, int width, int height, int event_mask);

  // Move and resize the passed-in window.
  // TODO: This isn't limited to input windows.
  bool ConfigureInputWindow(XWindow xid, int x, int y, int width, int height);

  // Get the X server's ID corresponding to the passed-in atom (the Atom
  // enum is defined in atom_cache.h).
  XAtom GetXAtom(Atom atom);

  // Get the name for an atom from the X server.
  const std::string& GetXAtomName(XAtom xatom);

  // Get the current time from the server.  This can be useful for e.g.
  // getting a timestamp to pass to XSetInputFocus() when triggered by an
  // event that doesn't contain a timestamp.
  XTime GetCurrentTimeFromServer();

  // Look up a window in 'client_windows_'.  The first version returns NULL
  // if the window doesn't exist, while the second crashes.
  Window* GetWindow(XWindow xid);
  Window* GetWindowOrDie(XWindow xid);

  // Do something reasonable with the input focus.
  // This is intended to be called by EventConsumers when they give up the
  // focus and aren't sure what to do with it.
  void TakeFocus();

  // Set the _NET_ACTIVE_WINDOW property, which contains the ID of the
  // currently-active window (in our case, this is the top-level window or
  // panel window that has the focus).
  bool SetActiveWindowProperty(XWindow xid);

  // Handle a change in the area available for the layout manager to use to
  // display toplevel windows.
  void HandleLayoutManagerAreaChange(int x, int y, int width, int height);

  // Register an event consumer as being interested in non-property-change
  // events on a particular window.
  void RegisterEventConsumerForWindowEvents(
      XWindow xid, EventConsumer* event_consumer);
  void UnregisterEventConsumerForWindowEvents(
      XWindow xid, EventConsumer* event_consumer);

  // Register an event consumer as a listener for changes of a particular
  // property on a particular window.  The consumer's
  // HandleWindowPropertyChange() method will be invoked whenever we
  // receive notification that the property has been changed (after we have
  // already handled the change).
  void RegisterEventConsumerForPropertyChanges(
      XWindow xid, XAtom xatom, EventConsumer* event_consumer);
  void UnregisterEventConsumerForPropertyChanges(
      XWindow xid, XAtom xatom, EventConsumer* event_consumer);

  // Register an event consumer as being interested in a particular type of
  // WmIpc message from Chrome.  The consumer's HandleChromeMessage()
  // method will be passed all messages of this type.
  void RegisterEventConsumerForChromeMessages(
      WmIpc::Message::Type message_type, EventConsumer* event_consumer);
  void UnregisterEventConsumerForChromeMessages(
      WmIpc::Message::Type message_type, EventConsumer* event_consumer);

 private:
  friend class BasicWindowManagerTest;
  friend class LayoutManagerTest;         // uses 'layout_manager_'
  friend class PanelTest;                 // uses 'panel_manager_'
  friend class PanelBarTest;              // uses 'panel_manager_'
  friend class PanelManagerTest;          // uses 'panel_manager_'
  FRIEND_TEST(LayoutManagerTest, Basic);  // uses TrackWindow()
  FRIEND_TEST(WindowTest, TransientFor);  // uses TrackWindow()
  FRIEND_TEST(WindowManagerTest, RegisterExistence);
  FRIEND_TEST(WindowManagerTest, EventConsumer);
  FRIEND_TEST(WindowManagerTest, RandR);

  typedef std::map<XWindow, std::set<EventConsumer*> > WindowEventConsumerMap;
  typedef std::map<std::pair<XWindow, XAtom>, std::set<EventConsumer*> >
      PropertyChangeEventConsumerMap;
  typedef std::map<WmIpc::Message::Type, std::set<EventConsumer*> >
      ChromeMessageEventConsumerMap;

  // Is this one of our internally-created windows?
  bool IsInternalWindow(XWindow xid) {
    return (xid == stage_xid_ || xid == overlay_xid_ || xid == wm_xid_);
  }

  // Get a manager selection as described in ICCCM section 2.8.  'atom' is
  // the selection to take, 'manager_win' is the window acquiring the
  // selection, and 'timestamp' is the current time.
  bool GetManagerSelection(
      XAtom atom, XWindow manager_win, XTime timestamp);

  // Tell the previous window and compositing managers to exit and register
  // ourselves as the new managers.
  bool RegisterExistence();

  // Set various one-time/unchanging properties on the root window as
  // specified in the Extended Window Manager Hints.
  bool SetEwmhGeneralProperties();

  // Set EWMH properties on the root window relating to the current screen
  // size (as stored in 'width_' and 'height_').
  bool SetEwmhSizeProperties();

  // Create a new ClutterGroup directly above 'bottom_actor'.  This is used
  // to create all of the '_depth' actors that are used for stacking.
  ClutterInterface::Actor* CreateActorAbove(
      ClutterInterface::Actor* bottom_actor);

  // Query the X server for all top-level windows and start tracking (and
  // possibly managing) them.
  bool ManageExistingWindows();

  // Start tracking this window (more specifically, create a Window object
  // for it and register it in 'client_windows_').  Returns NULL for
  // windows that we specifically shouldn't track (e.g. the Clutter stage
  // or the compositing overlay window).
  Window* TrackWindow(XWindow xid, bool override_redirect);

  // Handle a window getting mapped.  This is primarily used by
  // HandleMapNotify(), but is abstracted out into a separate method so
  // that ManageExistingWindows() can also use it to handle windows that
  // were already mapped when the WM started.
  void HandleMappedWindow(Window* win);

  // Set the WM_STATE property on a window.  Per ICCCM 4.1.3.1, 'state' can
  // be 0 (WithdrawnState), 1 (NormalState), or 3 (IconicState).  Per
  // 4.1.4, IconicState means that the top-level window isn't viewable, so
  // we should use NormalState even when drawing a scaled-down version of
  // the window.
  bool SetWmStateProperty(XWindow xid, int state);

  // Update the _NET_CLIENT_LIST and _NET_CLIENT_LIST_STACKING properties
  // on the root window (as described in EWMH).
  bool UpdateClientListProperty();
  bool UpdateClientListStackingProperty();

  // Handlers for various X events.
  void HandleButtonPress(const XButtonEvent& e);
  void HandleButtonRelease(const XButtonEvent& e);
  void HandleClientMessage(const XClientMessageEvent& e);
  void HandleConfigureNotify(const XConfigureEvent& e);
  void HandleConfigureRequest(const XConfigureRequestEvent& e);
  void HandleCreateNotify(const XCreateWindowEvent& e);
  void HandleDamageNotify(const XDamageNotifyEvent& e);
  void HandleDestroyNotify(const XDestroyWindowEvent& e);
  void HandleEnterNotify(const XEnterWindowEvent& e);
  void HandleFocusChange(const XFocusChangeEvent& e);
  void HandleKeyPress(const XKeyEvent& e);
  void HandleKeyRelease(const XKeyEvent& e);
  void HandleLeaveNotify(const XLeaveWindowEvent& e);
  void HandleMapNotify(const XMapEvent& e);
  void HandleMapRequest(const XMapRequestEvent& e);
  void HandleMappingNotify(const XMappingEvent& e);
  void HandleMotionNotify(const XMotionEvent& e);
  void HandlePropertyNotify(const XPropertyEvent& e);
  void HandleReparentNotify(const XReparentEvent& e);
  void HandleRRScreenChangeNotify(const XRRScreenChangeNotifyEvent& e);
  void HandleShapeNotify(const XShapeEvent& e);
  void HandleUnmapNotify(const XUnmapEvent& e);

  // Callback when the hotkey for launching an xterm is pressed.
  void LaunchTerminal();

  // Locks the screen by calling xscreensaver-command.
  void LockScreen();

  // Enable and configure an external monitor.
  void ConfigureExternalMonitor();

  // Callback to show or hide debugging information about client windows.
  void ToggleClientWindowDebugging();

  // Callback to show or hide the hotkey overlay images.
  void ToggleHotkeyOverlay();

  // Write a screenshot to disk.  If 'use_active_window' is true, the
  // screenshot will contain the currently-active client window's offscreen
  // pixmap.  Otherwise, the composited image from the root window will be
  // captured.
  void TakeScreenshot(bool use_active_window);

  // Helper method called repeatedly by a GLib timeout while the hotkey
  // overlay is being displayed to query the current keyboard state from
  // the X server and pass it to the overlay.
  static int QueryKeyboardStateThunk(void* data) {
    reinterpret_cast<WindowManager*>(data)->QueryKeyboardState();
    return 1;  // keep the timeout alive
  }
  void QueryKeyboardState();

  XConnection* xconn_;         // not owned
  ClutterInterface* clutter_;  // not owned

  XWindow root_;

  // Root window dimensions.
  int width_;
  int height_;

  // Offscreen window that we just use for registering as the WM.
  XWindow wm_xid_;

  ClutterInterface::StageActor* stage_;  // not owned
  scoped_ptr<ClutterInterface::Actor> background_;

  // Window containing the Clutter stage.
  XWindow stage_xid_;

  // XComposite overlay window.
  XWindow overlay_xid_;

  // Input window at the layer of the background image.  This exists solely
  // for the purpose of installing button grabs -- we can't install them on
  // the root window itself since they'd get activated by clicks in any of
  // the root's subwindows (this is apparently just how button grabs work
  // -- see the list of conditions in the XGrabButton() man page).
  XWindow background_xid_;

  scoped_ptr<StackingManager> stacking_manager_;

  // Windows that are being tracked.
  std::map<XWindow, std::tr1::shared_ptr<Window> > client_windows_;

  // This is a list of mapped, managed (i.e. not override-redirect) client
  // windows, in most-to-least-recently-mapped order.  Used to set EWMH's
  // _NET_CLIENT_LIST property.
  scoped_ptr<Stacker<XWindow> > mapped_xids_;

  // All immediate children of the root window (even ones that we don't
  // "track", in the sense of having Window objects for them in
  // 'client_windows_') in top-to-bottom stacking order.  EWMH's
  // _NET_CLIENT_LIST_STACKING property contains the managed (i.e. not
  // override-redirect) windows from this list.
  scoped_ptr<Stacker<XWindow> > stacked_xids_;

  // Things that consume events (e.g. LayoutManager, PanelManager, etc.).
  std::set<EventConsumer*> event_consumers_;

  // Map from windows to event consumers that will be notified if events
  // are received.
  WindowEventConsumerMap window_event_consumers_;

  // Map from (window, atom) pairs to event consumers that will be
  // notified if the corresponding property is changed.
  PropertyChangeEventConsumerMap property_change_event_consumers_;

  // Map from Chrome message types to event consumers that will receive
  // copies of the messages.
  ChromeMessageEventConsumerMap chrome_message_event_consumers_;

  // Windows about which the compositor has asked us to send events.
  std::set<XWindow> xids_tracked_by_compositor_;

  // Actors that are currently being used to debug client windows.
  std::vector<std::tr1::shared_ptr<ClutterInterface::Actor> >
      client_window_debugging_actors_;

  // The last window that was passed to SetActiveWindowProperty().
  XWindow active_window_xid_;

  scoped_ptr<AtomCache> atom_cache_;
  scoped_ptr<WmIpc> wm_ipc_;
  scoped_ptr<KeyBindings> key_bindings_;
  scoped_ptr<LayoutManager> layout_manager_;
  scoped_ptr<PanelManager> panel_manager_;
  scoped_ptr<MetricsReporter> metrics_reporter_;

  // GLib source ID for the timer that calls QueryKeyboardStateThunk().
  unsigned int query_keyboard_state_timer_;

  // Is the hotkey overlay currently being shown?
  bool showing_hotkey_overlay_;

  // Shows overlayed images containing hotkeys.
  scoped_ptr<HotkeyOverlay> hotkey_overlay_;

  // Version of the IPC protocol that Chrome is currently using.  See
  // WM_NOTIFY_IPC_VERSION in wm_ipc.h for details.
  int wm_ipc_version_;

  // Area currently available for the layout manager.
  int layout_manager_x_;
  int layout_manager_y_;
  int layout_manager_width_;
  int layout_manager_height_;

  DISALLOW_COPY_AND_ASSIGN(WindowManager);
};

}  // namespace window_manager

#endif  // WINDOW_MANAGER_WINDOW_MANAGER_H_
