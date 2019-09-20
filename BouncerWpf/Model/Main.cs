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
            set {
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

        private bool disposed_ = false;
        private bool Disposed {
            get {
                return disposed_;
            }
            set {
                disposed_ = value;
            }
        }

        #endregion
    }
}
