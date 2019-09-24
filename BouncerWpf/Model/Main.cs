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

        public ObservableCollection<Message> Messages { get; private set; } = new ObservableCollection<Message>();

        private Bouncer.Main native_;
        public Bouncer.Main Native {
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

        public string TimeReport {
            get {
                var stats = Native.GetStats();
                return String.Format(
                    "{0} / {1}",
                    Utilities.FormatDeltaTime(stats.totalViewTimeRecordedThisInstance),
                    Utilities.FormatDeltaTime(stats.totalViewTimeRecorded)
                );
            }
        }

        public string ViewersReport {
            get {
                var stats = Native.GetStats();
                return String.Format(
                    "{0} / {1} / {2}",
                    stats.currentViewerCount,
                    stats.maxViewerCountThisInstance,
                    stats.maxViewerCount
                );
            }
        }

        #endregion

        #region Public Methods

        public Main(Dispatcher dispatcher) {
            Dispatcher = dispatcher;
            Native = new Bouncer.Main();
            HostFacet = new Host(this);
            Native.StartApplication(HostFacet);
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
            NotifyPropertyChanged("TimeReport");
            NotifyPropertyChanged("ViewersReport");
        }

        #endregion

        #region Private Properties

        private Dispatcher Dispatcher { get; set; }
        private bool Disposed { get; set; } = false;
        private Host HostFacet { get; set; }
        private DispatcherTimer RefreshTimer { get; set; } = new DispatcherTimer();

        #endregion
    }
}
