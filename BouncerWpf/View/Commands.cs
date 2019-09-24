using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace Bouncer.Wpf.View {
    public static class Commands {
        #region Public Properties

        public static RoutedUICommand Configure { get; private set; }
        public static RoutedUICommand Exit { get; private set; }

        #endregion

        #region Public Methods

        static Commands() {
            Configure = new RoutedUICommand("_Configure", "Configure", typeof(Commands));
            var exitGestures = new InputGestureCollection();
            exitGestures.Add(new KeyGesture(Key.F4, ModifierKeys.Alt));
            Exit = new RoutedUICommand("E_xit", "Exit", typeof(Commands), exitGestures);
        }

        #endregion
    }
}
