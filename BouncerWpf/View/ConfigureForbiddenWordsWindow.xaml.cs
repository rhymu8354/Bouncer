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
    /// Interaction logic for ConfigureForbiddenWordsWindow.xaml
    /// </summary>
    public partial class ConfigureForbiddenWordsWindow: Window {
        #region Public Properties

        private Model.Configuration configuration_;
        public Model.Configuration Configuration {
            get {
                return configuration_;
            }
            set {
                if (Configuration != value) {
                    configuration_ = value;
                    DataContext = Configuration;
                }
            }
        }

        #endregion

        #region Public Methods

        public ConfigureForbiddenWordsWindow() {
            InitializeComponent();
        }

        #endregion

        #region Private Methods

        private void OnOK(object sender, RoutedEventArgs e) {
            Close();
        }

        private void OnSubmitForbiddenWord(object sender, ExecutedRoutedEventArgs e) {
            EditingItem = null;
        }

        private void OnDeleteForbiddenWord(object sender, ExecutedRoutedEventArgs e) {
            var forbiddenWord = (string)e.Parameter;
            Configuration.ForbiddenWords.Remove(forbiddenWord);
        }

        private void OnForbiddenWordClicked(object sender, MouseButtonEventArgs e) {
            if (e.ClickCount >= 2) {
                EditingItem = ((FrameworkElement)sender).Tag;
            }
        }

        private void OnForbiddenWordsSelectionChanged(object sender, SelectionChangedEventArgs e) {
            EditingItem = null;
        }

        private void OnNewForbiddenWord(object sender, ExecutedRoutedEventArgs e) {
            var forbiddenWord = NewForbiddenWord.Text;
            NewForbiddenWord.Text = "";
            Configuration.ForbiddenWords.Add(forbiddenWord);
        }


        #endregion

        #region PrivateProperties

        private object EditingItem {
            set {
                FileListItemDataTemplateSelector.ChosenItem = value;
                if (ForbiddenWords.ItemsSource != null) {
                    CollectionViewSource.GetDefaultView(ForbiddenWords.ItemsSource).Refresh();
                }
            }
        }

        #endregion
    }
}
