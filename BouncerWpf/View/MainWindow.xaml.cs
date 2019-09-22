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
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow: Window {
        #region Public Methods

        public MainWindow() {
            InitializeComponent();
            Model = new Model.Main();
            DataContext = Model;
        }

        #endregion

        #region Private Methods

        private void OnClosed(object sender, EventArgs e) {
            Model = null;
        }

        private void OnConfigure(object sender, ExecutedRoutedEventArgs e) {
            ConfigurationWindow configurationWindow = new ConfigurationWindow();
            configurationWindow.Model = Model;
            configurationWindow.ShowDialog();
        }

        private void OnExit(object sender, ExecutedRoutedEventArgs e) {
            Close();
        }

        #endregion

        #region Private Properties

        private Model.Main model_;
        private Model.Main Model {
            get {
                return model_;
            }
            set {
                if (Model != null) {
                    Model.Dispose();
                }
                model_ = value;
            }
        }

        #endregion
    }
}
