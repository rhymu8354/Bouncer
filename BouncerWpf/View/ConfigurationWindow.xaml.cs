using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace Bouncer.Wpf.View {
    /// <summary>
    /// Interaction logic for ConfigurationWindow.xaml
    /// </summary>
    public partial class ConfigurationWindow: Window {
        #region Public Properties

        private Model.Main model_;
        public Model.Main Model {
            get {
                return model_;
            }
            set {
                model_ = value;
            }
        }

        private Model.Configuration configuration_;
        public Model.Configuration Configuration {
            get {
                return configuration_;
            }
            private set {
                if (Configuration != value) {
                    if (Configuration != null) {
                        Configuration.Dispose();
                    }
                    configuration_ = value;
                    DataContext = Configuration;
                }
            }
        }

        #endregion

        #region Public Methods

        public ConfigurationWindow() {
            InitializeComponent();
        }

        #endregion

        #region Private Methods

        private void OnCancel(object sender, RoutedEventArgs e) {
            Close();
        }

        private void OnClosed(object sender, EventArgs e) {
            Configuration = null;
        }

        private void OnLoaded(object sender, RoutedEventArgs e) {
            Configuration = Model.Configuration;
        }

        private void OnSave(object sender, RoutedEventArgs e) {
            Model.Configuration = Configuration;
            Close();
        }

        #endregion

    }
}
