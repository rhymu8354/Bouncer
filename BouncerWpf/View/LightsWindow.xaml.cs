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
    /// Interaction logic for LightsWindow.xaml
    /// </summary>
    public partial class LightsWindow: Window {
        #region Public Properties

        private Model.Lights lights_;
        public Model.Lights Lights {
            get {
                return lights_;
            }
            set {
                lights_ = value;
                DataContext = Lights;
            }
        }

        #endregion

        #region Public Methods

        public LightsWindow() {
            InitializeComponent();
        }

        #endregion

        #region Private Methods

        #endregion

    }
}
