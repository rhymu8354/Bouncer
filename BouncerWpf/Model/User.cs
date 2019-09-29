using Bouncer.Wpf.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace Bouncer.Wpf.Model {
    public class User: INotifyPropertyChanged {
        #region Types

        public class UserTimeout {
            public User User { get; set; }
            public int Period { get; set; }
        }

        #endregion

        #region Public Properties

        public Brush Foreground {
            get {
                if (IsBanned) {
                    return Brushes.Red;
                } else if (Timeout > 0.0) {
                    return Brushes.OrangeRed;
                } else if (Watching) {
                    return Brushes.Blue;
                } else if (IsNewAccount) {
                    return Brushes.Green;
                } else {
                    return Brushes.Black;
                }
            }
        }

        public string BanMenuItemHeader {
            get {
                return String.Format("Ban {0}", Name);
            }
        }

        public Visibility BanMenuItemVisibility {
            get {
                return IsBanned ? Visibility.Collapsed : Visibility.Visible;
            }
        }

        public Bouncer.User.Bot Bot { get; private set; }
        public double CreatedAt { get; private set; }

        public string CreatedAtFormatted {
            get {
                return Utilities.FormatAbsoluteTime(CreatedAt);
            }
        }

        public string EditNoteMenuItemHeader {
            get {
                return "Edit Note";
            }
        }

        public double FirstMessageTime { get; private set; }

        public string FirstMessageTimeFormatted {
            get {
                return Utilities.FormatAbsoluteTime(FirstMessageTime);
            }
        }

        public string FirstMessageTimeReport {
            get {
                return (
                    Utilities.FormatAbsoluteTime(FirstMessageTimeThisInstance)
                    + " / "
                    + Utilities.FormatAbsoluteTime(FirstMessageTime)
                );
            }
        }

        public double FirstMessageTimeThisInstance { get; private set; }

        public string FirstMessageTimeThisInstanceFormatted {
            get {
                return Utilities.FormatAbsoluteTime(FirstMessageTimeThisInstance);
            }
        }

        public long Id { get; private set; }
        public bool IsBanned { get; private set; }
        public bool IsJoined { get; private set; }
        public bool IsNewAccount { get; private set; }
        public bool IsRecentChatter { get; private set; }
        public bool IsWhitelisted { get; private set; }
        public double JoinTime { get; private set; }

        public string JoinTimeFormatted {
            get {
                return Utilities.FormatAbsoluteTime(JoinTime);
            }
        }

        public double LastMessageTime { get; private set; }

        public string LastMessageTimeFormatted {
            get {
                return Utilities.FormatAbsoluteTime(LastMessageTime);
            }
        }

        public string Login { get; private set; }

        public string MarkBotMenuItemHeader {
            get {
                return "Bot";
            }
        }

        public Visibility MarkBotMenuItemVisibility {
            get {
                return (Bot == Bouncer.User.Bot.Yes) ? Visibility.Collapsed : Visibility.Visible;
            }
        }

        public string MarkNotBotMenuItemHeader {
            get {
                return "Not a bot";
            }
        }

        public Visibility MarkNotBotMenuItemVisibility {
            get {
                return (Bot == Bouncer.User.Bot.No) ? Visibility.Collapsed : Visibility.Visible;
            }
        }

        public string MarkPossibleBotMenuItemHeader {
            get {
                return "Possibly a bot";
            }
        }

        public Visibility MarkPossibleBotMenuItemVisibility {
            get {
                return (Bot == Bouncer.User.Bot.Unknown) ? Visibility.Collapsed : Visibility.Visible;
            }
        }

        public string Name { get; private set; }
        public uint NumMessages { get; private set; }
        public uint NumMessagesThisInstance { get; private set; }
        public string Note { get; private set; }

        public string NumMessagesReport {
            get {
                return String.Format(
                    "{0} / {1}",
                    NumMessagesThisInstance,
                    NumMessages
                );
            }
        }

        public double PartTime { get; private set; }

        public string PartTimeFormatted {
            get {
                return Utilities.FormatAbsoluteTime(PartTime);
            }
        }

        public Bouncer.User.Role Role { get; private set; }

        public string StartWatchingMenuItemHeader {
            get {
                return String.Format("Start Watching {0}", Name);
            }
        }

        public Visibility StartWatchingMenuItemVisibility {
            get {
                return Watching ? Visibility.Collapsed : Visibility.Visible;
            }
        }

        public string StopWatchingMenuItemHeader {
            get {
                return String.Format("Stop Watching {0}", Name);
            }
        }

        public Visibility StopWatchingMenuItemVisibility {
            get {
                return Watching ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        public double Timeout { get; private set; }

        public UserTimeout Timeout1s {
            get {
                var timeout = new UserTimeout {
                    User = this,
                    Period = 1
                };
                return timeout;
            }
        }

        public UserTimeout Timeout10m {
            get {
                var timeout = new UserTimeout {
                    User = this,
                    Period = 600
                };
                return timeout;
            }
        }

        public UserTimeout Timeout1h {
            get {
                var timeout = new UserTimeout {
                    User = this,
                    Period = 3600
                };
                return timeout;
            }
        }

        public UserTimeout Timeout6h {
            get {
                var timeout = new UserTimeout {
                    User = this,
                    Period = 21600
                };
                return timeout;
            }
        }

        public UserTimeout Timeout1d {
            get {
                var timeout = new UserTimeout {
                    User = this,
                    Period = 86400
                };
                return timeout;
            }
        }

        public string TimeOutMenuItemHeader {
            get {
                return String.Format("Time out {0}", Name);
            }
        }

        public string TimeOut1sMenuItemHeader {
            get {
                return String.Format("Time out {0} for 1 second", Name);
            }
        }

        public string TimeOut10mMenuItemHeader {
            get {
                return String.Format("Time out {0} for 10 minutes", Name);
            }
        }

        public string TimeOut1hMenuItemHeader {
            get {
                return String.Format("Time out {0} for 1 hour", Name);
            }
        }

        public string TimeOut6hMenuItemHeader {
            get {
                return String.Format("Time out {0} for 6 hours", Name);
            }
        }

        public string TimeOut1dMenuItemHeader {
            get {
                return String.Format("Time out {0} for 1 day", Name);
            }
        }

        public Visibility TimeOutMenuItemVisibility {
            get {
                return IsBanned ? Visibility.Collapsed : Visibility.Visible;
            }
        }

        public double TotalViewTime { get; private set; }

        public string TotalViewTimeFormatted {
            get {
                return Utilities.FormatDeltaTime(TotalViewTime);
            }
        }

        public string TimeoutFormatted {
            get {
                return Utilities.FormatDeltaTime(Timeout);
            }
        }

        public string UnbanMenuItemHeader {
            get {
                return String.Format("Unban {0}", Name);
            }
        }

        public Visibility UnbanMenuItemVisibility {
            get {
                return (
                    (
                        IsBanned
                        || (Timeout > 0.0)
                    )
                    ? Visibility.Visible
                    : Visibility.Collapsed
                );
            }
        }

        public string UnwhitelistMenuItemHeader {
            get {
                return String.Format("Unwhitelist {0}", Name);
            }
        }

        public Visibility UnwhitelistMenuItemVisibility {
            get {
                return IsWhitelisted ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        public bool Watching { get; private set; }

        public string WhitelistMenuItemHeader {
            get {
                return String.Format("Whitelist {0}", Name);
            }
        }

        public Visibility WhitelistMenuItemVisibility {
            get {
                return IsWhitelisted ? Visibility.Collapsed : Visibility.Visible;
            }
        }


        #endregion

        #region Public Methods

        public User(Bouncer.User native) {
            Bot = native.bot;
            CreatedAt = native.createdAt;
            FirstMessageTime = native.firstMessageTime;
            FirstMessageTimeThisInstance = native.firstMessageTimeThisInstance;
            Id = native.id;
            IsBanned = native.isBanned;
            IsJoined = native.isJoined;
            IsNewAccount = native.isNewAccount;
            IsRecentChatter = native.isRecentChatter;
            Watching = native.watching;
            IsWhitelisted = native.isWhitelisted;
            JoinTime = native.joinTime;
            LastMessageTime = native.lastMessageTime;
            Login = native.login;
            Name = native.name;
            Note = native.note;
            PartTime = native.partTime;
            TotalViewTime = native.totalViewTime;
            NumMessages = native.numMessages;
            NumMessagesThisInstance = native.numMessagesThisInstance;
            Role = native.role;
            Timeout = native.timeout;
        }

        public void Update(Bouncer.User native) {
            if (Bot != native.bot) {
                Bot = native.bot;
                NotifyPropertyChanged("Bot");
                NotifyPropertyChanged("MarkBotMenuItemVisibility");
                NotifyPropertyChanged("MarkNotBotMenuItemVisibility");
                NotifyPropertyChanged("MarkPossibleBotMenuItemVisibility");
            }
            if (CreatedAt != native.createdAt) {
                CreatedAt = native.createdAt;
                NotifyPropertyChanged("CreatedAt");
                NotifyPropertyChanged("CreatedAtFormatted");
            }
            if (FirstMessageTime != native.firstMessageTime) {
                FirstMessageTime = native.firstMessageTime;
                NotifyPropertyChanged("FirstMessageTime");
                NotifyPropertyChanged("FirstMessageTimeFormatted");
                if (FirstMessageTimeThisInstance == native.firstMessageTimeThisInstance) {
                    NotifyPropertyChanged("FirstMessageTimeReport");
                }
            }
            if (FirstMessageTimeThisInstance != native.firstMessageTimeThisInstance) {
                FirstMessageTimeThisInstance = native.firstMessageTimeThisInstance;
                NotifyPropertyChanged("FirstMessageTimeThisInstance");
                NotifyPropertyChanged("FirstMessageTimeReport");
            }
            if (Id != native.id) {
                Id = native.id;
                NotifyPropertyChanged("Id");
            }
            if (IsBanned != native.isBanned) {
                IsBanned = native.isBanned;
                NotifyPropertyChanged("IsBanned");
                NotifyPropertyChanged("Foreground");
                NotifyPropertyChanged("BanMenuItemVisibility");
                NotifyPropertyChanged("UnbanMenuItemVisibility");
            }
            if (IsNewAccount != native.isNewAccount) {
                IsNewAccount = native.isNewAccount;
                NotifyPropertyChanged("IsNewAccount");
                NotifyPropertyChanged("Foreground");
            }
            if (IsRecentChatter != native.isRecentChatter) {
                IsRecentChatter = native.isRecentChatter;
                NotifyPropertyChanged("IsRecentChatter");
                NotifyPropertyChanged("Foreground");
            }
            if (IsWhitelisted != native.isWhitelisted) {
                IsWhitelisted = native.isWhitelisted;
                NotifyPropertyChanged("IsWhitelisted");
                NotifyPropertyChanged("WhitelistMenuItemVisibility");
                NotifyPropertyChanged("UnwhitelistMenuItemVisibility");
            }
            if (IsJoined != native.isJoined) {
                IsJoined = native.isJoined;
                NotifyPropertyChanged("IsJoined");
            }
            if (Watching != native.watching) {
                Watching = native.watching;
                NotifyPropertyChanged("Watching");
                NotifyPropertyChanged("Foreground");
                NotifyPropertyChanged("StartWatchingMenuItemVisibility");
                NotifyPropertyChanged("StopWatchingMenuItemVisibility");
            }
            if (JoinTime != native.joinTime) {
                JoinTime = native.joinTime;
                NotifyPropertyChanged("JoinTime");
                NotifyPropertyChanged("JoinTimeFormatted");
            }
            if (LastMessageTime != native.lastMessageTime) {
                LastMessageTime = native.lastMessageTime;
                NotifyPropertyChanged("LastMessageTime");
                NotifyPropertyChanged("LastMessageTimeFormatted");
            }
            if (Login != native.login) {
                Login = native.login;
                NotifyPropertyChanged("Login");
            }
            if (Name != native.name) {
                Name = native.name;
                NotifyPropertyChanged("Name");
                NotifyPropertyChanged("BanMenuItemHeader");
                NotifyPropertyChanged("UnbanMenuItemHeader");
                NotifyPropertyChanged("StartWatchingMenuItemHeader");
                NotifyPropertyChanged("StopWatchingMenuItemHeader");
                NotifyPropertyChanged("UnwhitelistMenuItemHeader");
                NotifyPropertyChanged("WhitelistMenuItemHeader");
            }
            if (Note != native.note) {
                Note = native.note;
                NotifyPropertyChanged("Note");
            }
            if (PartTime != native.partTime) {
                PartTime = native.partTime;
                NotifyPropertyChanged("PartTime");
                NotifyPropertyChanged("PartTimeFormatted");
            }
            if (TotalViewTime != native.totalViewTime) {
                TotalViewTime = native.totalViewTime;
                NotifyPropertyChanged("TotalViewTime");
                NotifyPropertyChanged("TotalViewTimeFormatted");
            }
            if (NumMessages != native.numMessages) {
                NumMessages = native.numMessages;
                NotifyPropertyChanged("NumMessages");
                if (NumMessagesThisInstance != native.numMessagesThisInstance) {
                    NotifyPropertyChanged("NumMessagesReport");
                }
            }
            if (NumMessagesThisInstance != native.numMessagesThisInstance) {
                NumMessagesThisInstance = native.numMessagesThisInstance;
                NotifyPropertyChanged("NumMessagesThisInstance");
                NotifyPropertyChanged("NumMessagesReport");
            }
            if (Role != native.role) {
                Role = native.role;
                NotifyPropertyChanged("Role");
            }
            if (Timeout != native.timeout) {
                Timeout = native.timeout;
                NotifyPropertyChanged("Timeout");
                NotifyPropertyChanged("TimeoutFormatted");
                NotifyPropertyChanged("Foreground");
            }
        }

        #endregion

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler PropertyChanged;

        #endregion

        #region Private Methods

        private void NotifyPropertyChanged(string propertyName) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion
    }
}
