using Bouncer.Wpf.Properties;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;

namespace Bouncer.Wpf.Model {
    public class Main: INotifyPropertyChanged, IDisposable {

        #region Types

        private class Host: Bouncer.Host {
            public Host(Main main) {
                Main = main;
            }

            public override void StatusMessage(uint level, string message) {
                Trace.WriteLine(
                    String.Format(
                        "Status[{0}]: {1}",
                        level,
                        message
                    )
                );
                Main.Dispatcher.BeginInvoke(
                    DispatcherPriority.Normal,
                    (Action)(() => {
                        Main.Messages.Add(Message.StatusMessage(level, message));
                    })
                );
            }

            private Main Main { get; set; }
        }

        #endregion

        #region Public Properties

        public Configuration Configuration {
            get {
                return new Configuration(Native.GetConfiguration());
            }
            set {
                Native.SetConfiguration(value.Adaptee);
            }
        }

        public ObservableCollection<Message> Messages { get; private set; } = new ObservableCollection<Message>();

        public Bouncer.Stats stats_;
        public Bouncer.Stats Stats {
            get {
                return stats_;
            }
            private set {
                if (Stats == value) {
                    return;
                }
                if (Stats != null) {
                    Stats.Dispose();
                }
                stats_ = value;
                NotifyPropertyChanged("Stats");
                NotifyPropertyChanged("TimeReport");
                NotifyPropertyChanged("ViewersReport");
            }
        }

        public string TimeReport {
            get {
                if (Stats == null) {
                    return "";
                }
                return String.Format(
                    "{0} / {1}",
                    Utilities.FormatDeltaTime(Stats.totalViewTimeRecordedThisInstance),
                    Utilities.FormatDeltaTime(Stats.totalViewTimeRecorded)
                );
            }
        }

        public IEnumerable<User> Users {
            get {
                if (NativeUsers != null) {
                    foreach (var nativeUser in NativeUsers) {
                        bool show = false;
                        if (ShowAllViewers) {
                            show = true;
                        } else {
                            switch (nativeUser.bot) {
                                case Bouncer.User.Bot.Yes:
                                    if (ShowKnownBotViewers) show = true;
                                    break;

                                case Bouncer.User.Bot.No:
                                    if (ShowNonBotViewers) show = true;
                                    break;

                                default:
                                    if (ShowPossibleBotViewers) show = true;
                                    break;
                            }
                            if (!show) continue;
                            show = false;
                            if (nativeUser.isBanned) {
                                if (ShowBannedViewers) show = true;
                            } else if (nativeUser.timeout != 0.0) {
                                if (ShowTimedOutViewers) show = true;
                            } else {
                                if (ShowNonTimedOutViewers) show = true;
                            }
                            if (!show) continue;
                            show = false;
                            switch (nativeUser.role) {
                                case Bouncer.User.Role.Broadcaster:
                                case Bouncer.User.Role.Moderator:
                                    if (ShowModViewers) show = true;
                                    break;

                                case Bouncer.User.Role.VIP:
                                    if (ShowVipViewers) show = true;
                                    break;

                                case Bouncer.User.Role.Regular:
                                    if (ShowRegularViewers) show = true;
                                    break;

                                case Bouncer.User.Role.Pleb:
                                    if (ShowPlebViewers) show = true;
                                    break;

                                default:
                                    if (ShowUnknownViewers) show = true;
                                    break;
                            }
                            if (!show) continue;
                            show = false;
                            if (nativeUser.isJoined) {
                                if (ShowCurrentViewers) show = true;
                                if (nativeUser.numMessagesThisInstance == 0) {
                                    if (ShowLurkingViewers) show = true;
                                } else {
                                    if (ShowChattingViewers) show = true;
                                    if (
                                        ShowRecentlyChattingViewers
                                        && nativeUser.isRecentChatter
                                    ) {
                                        show = true;
                                    }
                                }
                            } else {
                                if (ShowMissingViewers) show = true;
                            }
                            if (!show) continue;
                            show = false;
                            if (Configuration.Whitelist.IndexOf(nativeUser.login) >= 0) {
                                if (ShowWhitelistedViewers) show = true;
                            } else {
                                if (ShowNonWhitelistedViewers) show = true;
                            }
                        }
                        if (show) {
                            yield return new User(nativeUser);
                        }
                    }
                }
            }
        }

        public string ViewersReport {
            get {
                if (Stats == null) {
                    return "";
                }
                return String.Format(
                    "{0} / {1} / {2}",
                    Stats.currentViewerCount,
                    Stats.maxViewerCountThisInstance,
                    Stats.maxViewerCount
                );
            }
        }

        private bool viewTimerRunning_ = false;
        public bool ViewTimerRunning {
            get {
                return viewTimerRunning_;
            }
            set {
                if (ViewTimerRunning == value) {
                    return;
                }
                viewTimerRunning_ = value;
                if (ViewTimerRunning) {
                    Native.StartViewTimer();
                } else {
                    Native.StopViewTimer();
                }
            }
        }

        private bool showAllViewers_ = true;
        public bool ShowAllViewers {
            get {
                return showAllViewers_;
            }
            set {
                if (ShowAllViewers == value) {
                    return;
                }
                showAllViewers_ = value;
                ShowKnownBotViewers = value;
                ShowPossibleBotViewers = value;
                ShowNonBotViewers = value;
                ShowBannedViewers = value;
                ShowTimedOutViewers = value;
                ShowNonTimedOutViewers = value;
                ShowModViewers = value;
                ShowVipViewers = value;
                ShowRegularViewers = value;
                ShowPlebViewers = value;
                ShowUnknownViewers = value;
                ShowCurrentViewers = value;
                ShowMissingViewers = value;
                ShowWhitelistedViewers = value;
                ShowNonWhitelistedViewers = value;
                NotifyPropertyChanged("ShowAllViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showKnownBotViewers_ = true;
        public bool ShowKnownBotViewers {
            get {
                return showKnownBotViewers_;
            }
            set {
                if (ShowKnownBotViewers == value) {
                    return;
                }
                showKnownBotViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowKnownBotViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showPossibleBotViewers_ = true;
        public bool ShowPossibleBotViewers {
            get {
                return showPossibleBotViewers_;
            }
            set {
                if (ShowPossibleBotViewers == value) {
                    return;
                }
                showPossibleBotViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowPossibleBotViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showNonBotViewers_ = true;
        public bool ShowNonBotViewers {
            get {
                return showNonBotViewers_;
            }
            set {
                if (ShowNonBotViewers == value) {
                    return;
                }
                showNonBotViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowNonBotViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showBannedViewers_ = true;
        public bool ShowBannedViewers {
            get {
                return showBannedViewers_;
            }
            set {
                if (ShowBannedViewers == value) {
                    return;
                }
                showBannedViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowBannedViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showTimedOutViewers_ = true;
        public bool ShowTimedOutViewers {
            get {
                return showTimedOutViewers_;
            }
            set {
                if (ShowTimedOutViewers == value) {
                    return;
                }
                showTimedOutViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowTimedOutViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showNonTimedOutViewers_ = true;
        public bool ShowNonTimedOutViewers {
            get {
                return showNonTimedOutViewers_;
            }
            set {
                if (ShowNonTimedOutViewers == value) {
                    return;
                }
                showNonTimedOutViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowNonTimedOutViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showModViewers_ = true;
        public bool ShowModViewers {
            get {
                return showModViewers_;
            }
            set {
                if (ShowModViewers == value) {
                    return;
                }
                showModViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowModViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showVipViewers_ = true;
        public bool ShowVipViewers {
            get {
                return showVipViewers_;
            }
            set {
                if (ShowVipViewers == value) {
                    return;
                }
                showVipViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowVipViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showRegularViewers_ = true;
        public bool ShowRegularViewers {
            get {
                return showRegularViewers_;
            }
            set {
                if (ShowRegularViewers == value) {
                    return;
                }
                showRegularViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowRegularViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showPlebViewers_ = true;
        public bool ShowPlebViewers {
            get {
                return showPlebViewers_;
            }
            set {
                if (ShowPlebViewers == value) {
                    return;
                }
                showPlebViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowPlebViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showUnknownViewers_ = true;
        public bool ShowUnknownViewers {
            get {
                return showUnknownViewers_;
            }
            set {
                if (ShowUnknownViewers == value) {
                    return;
                }
                showUnknownViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowUnknownViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showCurrentViewers_ = true;
        public bool ShowCurrentViewers {
            get {
                return showCurrentViewers_;
            }
            set {
                if (ShowCurrentViewers == value) {
                    return;
                }
                showCurrentViewers_ = value;
                ShowLurkingViewers = value;
                ShowChattingViewers = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowCurrentViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showMissingViewers_ = true;
        public bool ShowMissingViewers {
            get {
                return showMissingViewers_;
            }
            set {
                if (ShowMissingViewers == value) {
                    return;
                }
                showMissingViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowMissingViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showLurkingViewers_ = true;
        public bool ShowLurkingViewers {
            get {
                return showLurkingViewers_;
            }
            set {
                if (ShowLurkingViewers == value) {
                    return;
                }
                showLurkingViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    showCurrentViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                    NotifyPropertyChanged("ShowCurrentViewers");
                }
                NotifyPropertyChanged("ShowLurkingViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showChattingViewers_ = true;
        public bool ShowChattingViewers {
            get {
                return showChattingViewers_;
            }
            set {
                if (ShowChattingViewers == value) {
                    return;
                }
                showChattingViewers_ = value;
                ShowRecentlyChattingViewers = value;
                if (!value) {
                    showAllViewers_ = false;
                    showCurrentViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                    NotifyPropertyChanged("ShowCurrentViewers");
                }
                NotifyPropertyChanged("ShowChattingViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showRecentlyChattingViewers_ = true;
        public bool ShowRecentlyChattingViewers {
            get {
                return showRecentlyChattingViewers_;
            }
            set {
                if (ShowRecentlyChattingViewers == value) {
                    return;
                }
                showRecentlyChattingViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    showCurrentViewers_ = false;
                    showChattingViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                    NotifyPropertyChanged("ShowCurrentViewers");
                    NotifyPropertyChanged("ShowChattingViewers");
                }
                NotifyPropertyChanged("ShowRecentlyChattingViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showWhitelistedViewers_ = true;
        public bool ShowWhitelistedViewers {
            get {
                return showWhitelistedViewers_;
            }
            set {
                if (ShowWhitelistedViewers == value) {
                    return;
                }
                showWhitelistedViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowWhitelistedViewers");
                NotifyPropertyChanged("Users");
            }
        }

        private bool showNonWhitelistedViewers_ = true;
        public bool ShowNonWhitelistedViewers {
            get {
                return showNonWhitelistedViewers_;
            }
            set {
                if (ShowNonWhitelistedViewers == value) {
                    return;
                }
                showNonWhitelistedViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowNonWhitelistedViewers");
                NotifyPropertyChanged("Users");
            }
        }

        #endregion

        #region Public Methods

        public Main(Dispatcher dispatcher) {
            Dispatcher = dispatcher;
            Native = new Bouncer.Main();
            HostFacet = new Host(this);
            Native.StartApplication(HostFacet);
            Stats = Native.GetStats();
            NativeUsers = Native.GetUsers();
            RefreshTimer.Tick += OnRefreshTimerTick;
            RefreshTimer.Interval = new TimeSpan(0, 0, 1);
            RefreshTimer.Start();
        }

        #endregion

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler PropertyChanged;

        #endregion

        #region IDisposable

        public void Dispose() {
            Dispose(true);
        }

        #endregion

        #region Protected Methods

        protected virtual void Dispose(bool disposing) {
            if (!Disposed) {
                if (disposing) {
                    RefreshTimer.Tick -= OnRefreshTimerTick;
                    NativeUsers = null;
                    Stats = null;
                    Native = null;
                    HostFacet = null;
                }
                Disposed = true;
            }
        }

        #endregion

        #region Private Methods

        private void NotifyPropertyChanged(string propertyName) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private void OnRefreshTimerTick(object sender, EventArgs e) {
            Stats = Native.GetStats();
            NativeUsers = Native.GetUsers();
        }

        #endregion

        #region Private Properties

        private Dispatcher Dispatcher { get; set; }
        private bool Disposed { get; set; } = false;
        private Host HostFacet { get; set; }

        private Bouncer.Main native_;
        private Bouncer.Main Native {
            get {
                return native_;
            }
            set {
                if (Native != value) {
                    if (Native != null) {
                        Native.Dispose();
                    }
                    native_ = value;
                }
            }
        }

        private Bouncer.Users nativeUsers_;
        private Bouncer.Users NativeUsers {
            get {
                return nativeUsers_;
            }
            set {
                if (NativeUsers == value) {
                    return;
                }
                if (NativeUsers != null) {
                    NativeUsers.Dispose();
                }
                nativeUsers_ = value;
                NotifyPropertyChanged("Users");
            }
        }

        private DispatcherTimer RefreshTimer { get; set; } = new DispatcherTimer();

        #endregion
    }
}
