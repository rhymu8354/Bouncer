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
    public class UserNote: INotifyPropertyChanged {
        #region Public Properties

        private string name_;
        public string Name {
            get {
                return name_;
            }
            set {
                if (Name == value) {
                    return;
                }
                name_ = value;
                NotifyPropertyChanged("Name");
                NotifyPropertyChanged("Title");
            }
        }

        private string note_;
        public string Note {
            get {
                return note_;
            }
            set {
                if (Note == value) {
                    return;
                }
                note_ = value;
                NotifyPropertyChanged("Note");
            }
        }

        public string Title {
            get {
                return String.Format("Bouncer - Notes for {0}", Name);
            }
        }

        #endregion

        #region Public Methods

        public UserNote(User user) {
            Name = user.Name;
            Note = user.Note;
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
