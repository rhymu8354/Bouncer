using Bouncer.Wpf.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace Bouncer.Wpf.Model {
    public class User: IDisposable {
        #region Public Properties

        public Bouncer.User.Bot Bot {
            get {
                return Native.bot;
            }
        }

        public double CreatedAt {
            get {
                return Native.createdAt;
            }
        }

        public string CreatedAtFormatted {
            get {
                return Utilities.FormatAbsoluteTime(CreatedAt);
            }
        }

        public double FirstMessageTime {
            get {
                return Native.firstMessageTime;
            }
        }

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

        public double FirstMessageTimeThisInstance {
            get {
                return Native.firstMessageTimeThisInstance;
            }
        }

        public string FirstMessageTimeThisInstanceFormatted {
            get {
                return Utilities.FormatAbsoluteTime(FirstMessageTimeThisInstance);
            }
        }

        public long Id {
            get {
                return Native.id;
            }
        }

        public bool IsBanned {
            get {
                return Native.isBanned;
            }
        }

        public bool IsJoined {
            get {
                return Native.isJoined;
            }
        }

        public double JoinTime {
            get {
                return Native.joinTime;
            }
        }

        public string JoinTimeFormatted {
            get {
                return Utilities.FormatAbsoluteTime(JoinTime);
            }
        }

        public double LastMessageTime {
            get {
                return Native.lastMessageTime;
            }
        }

        public string LastMessageTimeFormatted {
            get {
                return Utilities.FormatAbsoluteTime(LastMessageTime);
            }
        }

        public string Login {
            get {
                return Native.login;
            }
        }

        public string Name {
            get {
                return Native.name;
            }
        }

        private Bouncer.User native_;
        public Bouncer.User Native {
            get {
                return native_;
            }
            private set {
                if (Native != value) {
                    if (Native != null) {
                        Native.Dispose();
                    }
                    native_ = value;
                }
            }
        }

        public double PartTime {
            get {
                return Native.partTime;
            }
        }

        public string PartTimeFormatted {
            get {
                return Utilities.FormatAbsoluteTime(PartTime);
            }
        }

        public double TotalViewTime {
            get {
                return Native.totalViewTime;
            }
        }

        public string TotalViewTimeFormatted {
            get {
                return Utilities.FormatDeltaTime(TotalViewTime);
            }
        }

        public uint NumMessages {
            get {
                return Native.numMessages;
            }
        }

        public uint NumMessagesThisInstance {
            get {
                return Native.numMessagesThisInstance;
            }
        }

        public string NumMessagesReport {
            get {
                return String.Format(
                    "{0} / {1}",
                    NumMessagesThisInstance,
                    NumMessages
                );
            }
        }

        public Bouncer.User.Role Role {
            get {
                return Native.role;
            }
        }

        public double Timeout {
            get {
                return Native.timeout;
            }
        }

        public string TimeoutFormatted {
            get {
                return Utilities.FormatAbsoluteTime(Timeout);
            }
        }

        #endregion

        #region Public Methods

        public User(Bouncer.User native) {
            Native = native;
        }

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
                    Native = null;
                }
                Disposed = true;
            }
        }

        #endregion

        #region Private Properties

        private bool Disposed { get; set; } = false;

        #endregion
    }
}
