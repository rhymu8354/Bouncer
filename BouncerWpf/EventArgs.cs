using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Bouncer.Wpf {

    public class EventArgs<T>: EventArgs {
        #region Public Properties

        public T Args {
            get;
            set;
        }

        #endregion

        #region Public Methods

        public EventArgs(T args) {
            Args = args;
        }

        #endregion
    }

}
