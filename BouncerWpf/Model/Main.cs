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
using System.Windows.Data;
using System.Windows.Threading;

namespace Bouncer.Wpf.Model {
    public class Main: INotifyPropertyChanged, IDisposable {

        #region Types

        private class Host: Bouncer.Host {
            public Host(Main main) {
                Main = main;
            }

            public override void StatusMessage(uint level, string message, long userid) {
                Trace.WriteLine(
                    String.Format(
                        "Status[{0}/{1}]: {2}",
                        level,
                        userid,
                        message
                    )
                );
                Main.Dispatcher.BeginInvoke(
                    DispatcherPriority.Normal,
                    (Action)(() => {
                        Main.Messages.Add(Message.StatusMessage(level, message, userid));
                        Main.DiscardOldMessages();
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

        private Lights lights_ = new Lights();
        public Lights Lights {
            get {
                return lights_;
            }
        }

        private int maxMessages_ = 1000;
        public int MaxMessages {
            get {
                return maxMessages_;
            }
            set {
                if (MaxMessages == value) {
                    return;
                }
                maxMessages_ = value;
                NotifyPropertyChanged("MaxMessages");
                DiscardOldMessages();
            }
        }

        public ObservableCollection<Message> Messages { get; private set; } = new ObservableCollection<Message>();

        private Message selectedMessage_;
        public Message SelectedMessage {
            get {
                return selectedMessage_;
            }
            set {
                if (SelectedMessage == value) {
                    return;
                }
                selectedMessage_ = value;
                NotifyPropertyChanged("SelectedUser");
            }
        }


        private User selectedUser_;
        public User SelectedUser {
            get {
                return selectedUser_;
            }
            set {
                if (SelectedUser == value) {
                    return;
                }
                selectedUser_ = value;
                NotifyPropertyChanged("SelectedUser");
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
                ShowPlebViewers = value;
                ShowUnknownViewers = value;
                ShowCurrentViewers = value;
                ShowMissingViewers = value;
                ShowWhitelistedViewers = value;
                ShowNonWhitelistedViewers = value;
                ShowWatchedViewers = value;
                ShowUnwatchedViewers = value;
                ShowNewAccounts = value;
                ShowOldAccounts = value;
                NotifyPropertyChanged("ShowAllViewers");
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
            }
        }

        private bool showNewAccounts_ = true;
        public bool ShowNewAccounts {
            get {
                return showNewAccounts_;
            }
            set {
                if (ShowNewAccounts == value) {
                    return;
                }
                showNewAccounts_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowNewAccounts");
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
            }
        }

        private bool showOldAccounts_ = true;
        public bool ShowOldAccounts {
            get {
                return showOldAccounts_;
            }
            set {
                if (ShowOldAccounts == value) {
                    return;
                }
                showOldAccounts_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowOldAccounts");
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
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
                RefreshUsers();
            }
        }

        private bool showUnwatchedViewers_ = true;
        public bool ShowUnwatchedViewers {
            get {
                return showUnwatchedViewers_;
            }
            set {
                if (ShowUnwatchedViewers == value) {
                    return;
                }
                showUnwatchedViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowUnwatchedViewers");
                RefreshUsers();
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
                RefreshUsers();
            }
        }

        private bool showWatchedViewers_ = true;
        public bool ShowWatchedViewers {
            get {
                return showWatchedViewers_;
            }
            set {
                if (ShowWatchedViewers == value) {
                    return;
                }
                showWatchedViewers_ = value;
                if (!value) {
                    showAllViewers_ = false;
                    NotifyPropertyChanged("ShowAllViewers");
                }
                NotifyPropertyChanged("ShowWatchedViewers");
                RefreshUsers();
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
                RefreshUsers();
            }
        }

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

        public ObservableCollection<User> Users { get; private set; } = new ObservableCollection<User>();

        public Dictionary<long, User> UsersById = new Dictionary<long, User>();

        public string ViewersReport {
            get {
                if (Stats == null) {
                    return "";
                }
                return String.Format(
                    "{0} / {1} / {2} / {3}",
                    Stats.currentViewerCount,
                    Stats.maxViewerCountThisInstance,
                    Stats.maxViewerCount,
                    Stats.numViewersKnown
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
                NotifyPropertyChanged("ViewTimerRunning");
            }
        }

        #endregion

        #region Public Methods

        public Main(Dispatcher dispatcher) {
            Dispatcher = dispatcher;
            Native = new Bouncer.Main();
            HostFacet = new Host(this);
            Native.StartApplication(HostFacet);
            Lights.PropertyChanged += OnLightsPropertyChanged;
            Stats = Native.GetStats();
            var usersListLiveShaping = CollectionViewSource.GetDefaultView(Users) as ICollectionViewLiveShaping;
            if (usersListLiveShaping != null) {
                usersListLiveShaping.IsLiveSorting = true;
            }
            RefreshUsers();
            RefreshTimer.Tick += OnRefreshTimerTick;
            RefreshTimer.Interval = new TimeSpan(0, 0, 1);
            RefreshTimer.Start();
        }

        public void Ban(User user) {
            Native.Ban(user.Id);
        }

        public void MarkGreeted(User user) {
            Native.MarkGreeted(user.Id);
        }

        public void QueryChannelStats() {
            Native.QueryChannelStats();
        }

        public void SetBotStatus(User user, Bouncer.User.Bot bot) {
            Native.SetBotStatus(user.Id, bot);
        }

        public void SetNote(User user, string note) {
            Native.SetNote(user.Id, note);
        }

        public void StartWatching(User user) {
            Native.StartWatching(user.Id);
        }

        public void StopWatching(User user) {
            Native.StopWatching(user.Id);
        }

        public void TimeOut(User user, int seconds) {
            Native.TimeOut(user.Id, seconds);
        }

        public void Unban(User user) {
            Native.Unban(user.Id);
        }

        public void Unwhitelist(User user) {
            Native.Unwhitelist(user.Id);
        }

        public void Whitelist(User user) {
            Native.Whitelist(user.Id);
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
                    Lights.PropertyChanged -= OnLightsPropertyChanged;
                    RefreshTimer.Tick -= OnRefreshTimerTick;
                    Stats = null;
                    Native = null;
                    HostFacet = null;
                }
                Disposed = true;
            }
        }

        #endregion

        #region Private Methods

        private void DiscardOldMessages() {
            while (Messages.Count > MaxMessages) {
                Messages.RemoveAt(0);
            }
        }

        private bool FilterUser(Bouncer.User user) {
            bool show = false;
            if (ShowAllViewers) {
                return true;
            }
            switch (user.bot) {
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
            if (!show) return false;
            show = false;
            if (user.isBanned) {
                if (ShowBannedViewers) show = true;
            } else if (user.timeout != 0.0) {
                if (ShowTimedOutViewers) show = true;
            } else {
                if (ShowNonTimedOutViewers) show = true;
            }
            if (!show) return false;
            show = false;
            switch (user.role) {
                case Bouncer.User.Role.Staff:
                case Bouncer.User.Role.Admin:
                case Bouncer.User.Role.Broadcaster:
                case Bouncer.User.Role.Moderator:
                    if (ShowModViewers) show = true;
                    break;

                case Bouncer.User.Role.VIP:
                    if (ShowVipViewers) show = true;
                    break;

                case Bouncer.User.Role.Pleb:
                    if (ShowPlebViewers) show = true;
                    break;

                default:
                    if (ShowUnknownViewers) show = true;
                    break;
            }
            if (!show) return false;
            show = false;
            if (user.isJoined) {
                if (ShowCurrentViewers) show = true;
                if (user.numMessagesThisInstance == 0) {
                    if (ShowLurkingViewers) show = true;
                } else {
                    if (ShowChattingViewers) show = true;
                    if (
                        ShowRecentlyChattingViewers
                        && user.isRecentChatter
                    ) {
                        show = true;
                    }
                }
            } else {
                if (ShowMissingViewers) show = true;
            }
            if (!show) return false;
            show = false;
            if (user.isWhitelisted) {
                if (ShowWhitelistedViewers) show = true;
            } else {
                if (ShowNonWhitelistedViewers) show = true;
            }
            if (!show) return false;
            show = false;
            if (user.watching) {
                if (ShowWatchedViewers) show = true;
            } else {
                if (ShowUnwatchedViewers) show = true;
            }
            if (!show) return false;
            show = false;
            if (user.isNewAccount) {
                if (ShowNewAccounts) show = true;
            } else {
                if (ShowOldAccounts) show = true;
            }
            return show;
        }

        private void NotifyPropertyChanged(string propertyName) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private void OnLightsPropertyChanged(object sender, PropertyChangedEventArgs e) {
            if (Lights.On) {
                Native.LightsOn(
                    Lights.Red,
                    Lights.Green,
                    Lights.Blue,
                    Lights.Brightness
                );
            } else if (e.PropertyName == "On") {
                Native.LightsOff();
            }
        }

        private void OnRefreshTimerTick(object sender, EventArgs e) {
            Stats = Native.GetStats();
            RefreshUsers();
        }

        private void RefreshUsers() {
            var nativeUsers = Native.GetUsers();
            var newUsersById = new Dictionary<long, Bouncer.User>();
            foreach (var user in nativeUsers) {
                newUsersById[user.id] = user;
            }
            var numOldUsers = Users.Count;
            int i = 0;
            while (i < numOldUsers) {
                var oldUser = Users[i];
                Bouncer.User user;
                if (newUsersById.TryGetValue(oldUser.Id, out user)) {
                    if (FilterUser(user)) {
                        oldUser.Update(user);
                        ++i;
                    } else {
                        Users.RemoveAt(i);
                        UsersById.Remove(oldUser.Id);
                        --numOldUsers;
                    }
                    newUsersById.Remove(oldUser.Id);
                } else {
                    Users.RemoveAt(i);
                    UsersById.Remove(oldUser.Id);
                    --numOldUsers;
                }
            }
            foreach (var user in newUsersById.Values) {
                if (FilterUser(user)) {
                    var newUser = new User(user);
                    Users.Add(newUser);
                    UsersById.Add(newUser.Id, newUser);
                }
            }
            nativeUsers.Dispose();
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

        private DispatcherTimer RefreshTimer { get; set; } = new DispatcherTimer();

        #endregion
    }
}
