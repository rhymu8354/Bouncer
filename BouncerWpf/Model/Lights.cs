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
    public class Lights: INotifyPropertyChanged {

        #region Public Properties

        private bool on_ = false;
        public bool On {
            get {
                return on_;
            }
            set {
                if (On == value) {
                    return;
                }
                on_ = value;
                NotifyPropertyChanged("On");
                NotifyPropertyChanged("Off");
            }
        }

        public bool Off {
            get {
                return !On;
            }
            set {
                On = !value;
            }
        }

        private double red_ = Settings.Default.LastLightsRed;
        public double Red {
            get {
                return red_;
            }
            set {
                if (Red == value) {
                    return;
                }
                red_ = value;
                Settings.Default.LastLightsRed = Red;
                Settings.Default.Save();
                NotifyPropertyChanged("Red");
            }
        }

        private double green_ = Settings.Default.LastLightsGreen;
        public double Green {
            get {
                return green_;
            }
            set {
                if (Green == value) {
                    return;
                }
                green_ = value;
                Settings.Default.LastLightsGreen = Green;
                Settings.Default.Save();
                NotifyPropertyChanged("Green");
            }
        }

        private double blue_ = Settings.Default.LastLightsBlue;
        public double Blue {
            get {
                return blue_;
            }
            set {
                if (Blue == value) {
                    return;
                }
                blue_ = value;
                Settings.Default.LastLightsBlue = Blue;
                Settings.Default.Save();
                NotifyPropertyChanged("Blue");
            }
        }

        private double brightness_ = Settings.Default.LastLightsBrightness;
        public double Brightness {
            get {
                return brightness_;
            }
            set {
                if (Brightness == value) {
                    return;
                }
                brightness_ = value;
                Settings.Default.LastLightsBrightness = Brightness;
                Settings.Default.Save();
                NotifyPropertyChanged("Brightness");
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
