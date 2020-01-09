using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace Bouncer.Wpf.View {
    public class ChosenItemDataTemplateSelector: DataTemplateSelector {
        #region Public Properties

        public DataTemplate NormalItemTemplate {
            get;
            set;
        }

        public DataTemplate ChosenItemTemplate {
            get;
            set;
        }

        public object chosenItem_;
        public object ChosenItem {
            get {
                return chosenItem_;
            }
            set {
                chosenItem_ = value;
            }
        }

        #endregion

        #region DataTemplateSelector

        public override DataTemplate SelectTemplate(object item, DependencyObject container) {
            if (item == ChosenItem) {
                return ChosenItemTemplate;
            } else {
                return NormalItemTemplate;
            }
        }

        #endregion

        #region Public Properties

        #endregion
    }
}
