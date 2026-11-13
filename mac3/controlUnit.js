import ControlUnit1 from '../mac1/controlUnit.js';

class ControlUnit3 extends ControlUnit1 {
    constructor(cacheSize = 3) {
        super();
        this.cacheSize = cacheSize;
        this.cache = {
            getSnapshot: () => []
        };
    }

    setCacheSize(size) {
        this.cacheSize = size;
    }
}

export default ControlUnit3;
