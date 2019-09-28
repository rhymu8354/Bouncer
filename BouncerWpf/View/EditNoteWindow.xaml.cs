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
    /// Interaction logic for EditNoteWindow.xaml
    /// </summary>
    public partial class EditNoteWindow: Window {
        #region Public Properties

        private Model.UserNote model_;
        public Model.UserNote Model {
            get {
                return model_;
            }
            set {
                model_ = value;
                DataContext = Model;
            }
        }

        #endregion

        #region Public Methods

        public EditNoteWindow() {
            InitializeComponent();
        }

        #endregion

        #region Private Methods

        private void OnCancel(object sender, RoutedEventArgs e) {
            DialogResult = false;
            Close();
        }

        private void OnOK(object sender, RoutedEventArgs e) {
            DialogResult = true;
            Close();
        }

        #endregion

    }
}
