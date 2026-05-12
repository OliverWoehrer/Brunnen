import { ref, computed } from 'vue';

const BREAKPOINTS = { small: 0, medium: 601, large: 993 };

const width = ref(window.innerWidth);
let initialized = false;

function update() {
    width.value = window.innerWidth;
}

function init() {
    if(initialized) return;
    window.addEventListener('resize', update);
    initialized = true;
}

function nextBreakpoint(size) {
    if (size === 'small') return BREAKPOINTS.medium;
    if (size === 'medium') return BREAKPOINTS.large;
    return Infinity;
}

export function useScreenSize() {
    init();

    const screenSize = computed(() => {
        if (width.value < BREAKPOINTS.medium) return 'small';
        if (width.value < BREAKPOINTS.large) return 'medium';
        return 'large';
    });

    const isAtLeast = (size) => computed(() => width.value >= BREAKPOINTS[size]);

    const isLargerThan = (size) => computed(() => width.value > BREAKPOINTS[size]);

    const isSmallerThan = (size) => computed(() => width.value < BREAKPOINTS[size]);

    const isAtMost = (size) => computed(() => width.value < nextBreakpoint(size));

    return {
        width,
        screenSize,
        isAtLeast,
        isAtMost,
        isLargerThan,
        isSmallerThan,
    };
}