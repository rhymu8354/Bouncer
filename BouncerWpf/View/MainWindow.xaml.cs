using System;
using System.Collections.Generic;
using System.ComponentModel;
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
            Model = new Model.Main(Dispatcher);
            DataContext = Model;
        }

        #endregion

        #region Private Methods

        private void OnBan(object sender, ExecutedRoutedEventArgs e) {
            var user = e.Parameter as Model.User;
            if (user == null) {
                return;
            }
            Model.Ban(user);
        }

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

        private void OnSortUsers(object sender, RoutedEventArgs e) {
            var newSortHeader = sender as GridViewColumnHeader;
            var tag = newSortHeader.Tag.ToString();
            UsersList.Items.SortDescriptions.Clear();
            ListSortDirection sortDirection = (
                (
                    (tag == "Role")
                    || (tag == "CreatedAt")
                    || (tag == "TotalViewTime")
                    || (tag == "JoinTime")
                    || (tag == "PartTime")
                    || (tag == "LastMessageTime")
                    || (tag == "NumMessages")
                    || (tag == "IsJoined")
                )
                ? ListSortDirection.Descending
                : ListSortDirection.Ascending
            );
            if (SortHeader != null) {
                AdornerLayer.GetAdornerLayer(SortHeader).Remove(UserListSortAdorner);
            }
            if (newSortHeader == SortHeader) {
                sortDirection = (
                    (UserListSortAdorner.Direction == ListSortDirection.Ascending)
                    ? ListSortDirection.Descending
                    : ListSortDirection.Ascending
                );
            } else {
                SortHeader = newSortHeader;
            }
            UserListSortAdorner = new SortAdorner(SortHeader, sortDirection);
            AdornerLayer.GetAdornerLayer(SortHeader).Add(UserListSortAdorner);
            UsersList.Items.SortDescriptions.Add(new SortDescription(tag, sortDirection));
        }

        private void OnUnban(object sender, ExecutedRoutedEventArgs e) {
            var user = e.Parameter as Model.User;
            if (user == null) {
                return;
            }
            Model.Unban(user);
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

        private GridViewColumnHeader SortHeader { get; set; }
        private SortAdorner UserListSortAdorner { get; set; }

        #endregion
    }
}
