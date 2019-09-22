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
    public class Main: INotifyPropertyChanged, IDisposable {

        #region Types

        private class Host: Bouncer.Host {
            public Host(Main main) {
                main_ = main;
            }

            public override int Foo() => 42;

            private readonly Main main_;
        }

        #endregion

        #region Public Properties

        private string status_;
        public string Status {
            get {
                return status_;
            }
            set {
                if (Status != value) {
                    status_ = value;
                    NotifyPropertyChanged("Status");
                }
            }
        }

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

        public Main() {
            Native = new Bouncer.Main();
            HostFacet = new Host(this);
            Native.SetHost(HostFacet);
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

        #endregion
    }
}
