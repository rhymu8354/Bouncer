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

namespace Bouncer.Wpf.Model {
    public class Configuration: INotifyPropertyChanged, IDisposable {

        #region Public Properties

        private Bouncer.Configuration adaptee_;
        public Bouncer.Configuration Adaptee {
            get {
                return adaptee_;
            }
            private set {
                if (Adaptee != value) {
                    if (Adaptee != null) {
                        Adaptee.Dispose();
                    }
                    adaptee_ = value;
                }
            }
        }

        public string Account {
            get {
                return Adaptee.account;
            }
            set {
                Adaptee.account = value;
                NotifyPropertyChanged("Account");
            }
        }

        public bool AutoBanTitleScammers {
            get {
                return Adaptee.autoBanTitleScammers;
            }
            set {
                Adaptee.autoBanTitleScammers = value;
                NotifyPropertyChanged("AutoBanTitleScammers");
            }
        }

        public bool AutoBanForbiddenWords {
            get {
                return Adaptee.autoBanForbiddenWords;
            }
            set {
                Adaptee.autoBanForbiddenWords = value;
                NotifyPropertyChanged("AutoBanForbiddenWords");
            }
        }

        public bool AutoTimeOutNewAccountChatters {
            get {
                return Adaptee.autoTimeOutNewAccountChatters;
            }
            set {
                Adaptee.autoTimeOutNewAccountChatters = value;
                NotifyPropertyChanged("AutoTimeOutNewAccountChatters");
            }
        }

        public string Token {
            get {
                return Adaptee.token;
            }
            set {
                Adaptee.token = value;
                NotifyPropertyChanged("Token");
            }
        }

        public string ClientId {
            get {
                return Adaptee.clientId;
            }
            set {
                Adaptee.clientId = value;
                NotifyPropertyChanged("ClientId");
            }
        }

        public string Channel {
            get {
                return Adaptee.channel;
            }
            set {
                Adaptee.channel = value;
                NotifyPropertyChanged("Channel");
            }
        }

        public ObservableStdVector<StdVectorString, string> ForbiddenWords {
            get;
            private set;
        }

        public string GreetingPattern {
            get {
                return Adaptee.greetingPattern;
            }
            set {
                Adaptee.greetingPattern = value;
                NotifyPropertyChanged("GreetingPattern");
            }
        }

        public uint MinDiagnosticsThreshold {
            get {
                return Adaptee.minDiagnosticsLevel;
            }
            set {
                Adaptee.minDiagnosticsLevel = value;
                NotifyPropertyChanged("MinDiagnosticsThreshold");
            }
        }

        public double NewAccountAgeThreshold {
            get {
                return Adaptee.newAccountAgeThreshold;
            }
            set {
                Adaptee.newAccountAgeThreshold = value;
                NotifyPropertyChanged("NewAccountAgeThreshold");
            }
        }

        public string NewAccountChatterTimeoutExplanation {
            get {
                return Adaptee.newAccountChatterTimeoutExplanation;
            }
            set {
                Adaptee.newAccountChatterTimeoutExplanation = value;
                NotifyPropertyChanged("NewAccountChatterTimeoutExplanation");
            }
        }

        public double RecentChatThreshold {
            get {
                return Adaptee.recentChatThreshold;
            }
            set {
                Adaptee.recentChatThreshold = value;
                NotifyPropertyChanged("RecentChatThreshold");
            }
        }

        #endregion

        #region Public Methods

        public Configuration(Bouncer.Configuration adaptee) {
            Adaptee = adaptee;
            ForbiddenWords = new ObservableStdVector<StdVectorString, string>(Adaptee.forbiddenWords);
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
                    Adaptee = null;
                }
                Disposed = true;
            }
        }

        #endregion

        #region Private Methods

        private void NotifyPropertyChanged(string propertyName) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion

        #region Private Properties

        private bool Disposed { get; set; } = false;

        #endregion
    }
}
