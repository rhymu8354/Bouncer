using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace Bouncer.Wpf.View {
    public static class Commands {
        #region Public Properties

        public static RoutedUICommand Ban { get; private set; }
        public static RoutedUICommand Configure { get; private set; }
        public static RoutedUICommand Exit { get; private set; }
        public static RoutedUICommand MarkBot { get; private set; }
        public static RoutedUICommand MarkNotBot { get; private set; }
        public static RoutedUICommand MarkPossibleBot { get; private set; }
        public static RoutedUICommand Unban { get; private set; }
        public static RoutedUICommand Unwhitelist { get; private set; }
        public static RoutedUICommand Whitelist { get; private set; }

        #endregion

        #region Public Methods

        static Commands() {
            Ban = new RoutedUICommand("Ban", "Ban", typeof(Commands));
            Configure = new RoutedUICommand("_Configure", "Configure", typeof(Commands));
            var exitGestures = new InputGestureCollection();
            exitGestures.Add(new KeyGesture(Key.F4, ModifierKeys.Alt));
            Exit = new RoutedUICommand("E_xit", "Exit", typeof(Commands), exitGestures);
            MarkBot = new RoutedUICommand("Mark As Bot", "MarkBot", typeof(Commands));
            MarkNotBot = new RoutedUICommand("Mark As Not a Bot", "MarkNotBot", typeof(Commands));
            MarkPossibleBot = new RoutedUICommand("Mark As a Possible Bot", "MarkPossibleBot", typeof(Commands));
            Unban = new RoutedUICommand("Unban", "Unban", typeof(Commands));
            Unwhitelist = new RoutedUICommand("Unwhitelist", "Unwhitelist", typeof(Commands));
            Whitelist = new RoutedUICommand("Whitelist", "Whitelist", typeof(Commands));
        }

        #endregion
    }
}
