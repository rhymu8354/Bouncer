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
    /// Interaction logic for UserWindow.xaml
    /// </summary>
    public partial class UserWindow: Window {
        #region Public Properties

        private Model.Main main_;
        public Model.Main Main {
            get {
                return main_;
            }
            set {
                main_ = value;
            }
        }

        private Model.User user_;
        public Model.User User {
            get {
                return user_;
            }
            set {
                user_ = value;
                DataContext = User;
            }
        }

        #endregion

        #region Public Methods

        public UserWindow() {
            InitializeComponent();
        }

        #endregion

        #region Private Methods

        private void OnSaveNotes(object sender, RoutedEventArgs e) {
            Main.SetNote(User, Note.Text);
        }

        #endregion

    }
}
