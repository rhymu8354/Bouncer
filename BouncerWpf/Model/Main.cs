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

        #endregion

        #region Public Methods

        public Main(Dispatcher dispatcher) {
            Dispatcher = dispatcher;
            Native = new Bouncer.Main();
            HostFacet = new Host(this);
            Native.Start(HostFacet);
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

        #endregion

        #region Private Properties

        private bool Disposed { get; set; } = false;
        private Host HostFacet { get; set; }
        private Dispatcher Dispatcher { get; set; }

        #endregion
    }
}
