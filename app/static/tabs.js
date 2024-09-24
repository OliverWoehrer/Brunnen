/**
 * [Tab Controlls]
 * This file provides functionality to open and close tabs by hidding and showing them when
 * clicked. There is a seperation between Links and Tabs for maximum flexibility. Links are always
 * visible (e.g. a bar). Tabs are displayed if the matching link is clicked.
 * 
 * Link Spaces are containers for links, use CSS class "linkspace" for those elements. Use the CSS
 * class "link" for Links. Tab Spaces are containers for tabs, use CSS class "tabspace" for those
 * elements. Use the CSS class "tab" for Tabs.
 */

/* Example Code: */
/* 
<div class="linkspace">
    <div class="link" style="width: auto;">
        <button onclick="openTab(event, 'first_tabname')">First</button>
    </div>
    <div class="link" style="width: auto;">
        <button onclick="openTab(event, 'second_tabname')">Second</button>
    </div>
    <div class="link" style="overflow: hidden;">
        <div>&nbsp;</div>
    </div>
</div>

<div class="tabspace">
    <div id="first_tabname" class="tab">
        First Tab
    </div>
    <div id="first_tabname" class="tab">
        Second Tab
    </div>
</div>

<script src="tabs.js"></script>
<script>
    initTabs();
</script>
 
*/

function styleSelected(link) {
    link.style.borderBottomStyle = "solid";
}

function styleDefault(link) {
    link.style.borderBottomStyle = "none";
}

function openTab(event, tabName) {
    // Find Other Links:
    currentElement = event.currentTarget
    isLink = currentElement.classList.contains("link");
    while(!isLink) { // climb up the element hierarchy until element is a link
        currentElement = currentElement.parentElement;
        isLink = currentElement.classList.contains("link");
    }
    
    // Look For Sibling Elements (Links):
    linkspace = currentElement.parentElement;
    const links = linkspace.getElementsByClassName("link");

    // Style All Links (Default):
    for(const link of links) {
        styleDefault(link);
    }

    // Style Target Link:
    targetLink = currentElement;
    styleSelected(targetLink);

    // Find Other Tabs:
    targetTab = document.getElementById(tabName);
    tabspace = targetTab;
    isTabSpace = false;
    while(!isTabSpace) { // climb up the element hierarchy until element is a tabspace
        tabspace = tabspace.parentElement;
        isTabSpace = tabspace.classList.contains("tabspace");
    }

    // Hide Other Tabs (Default):
    const tabs = tabspace.getElementsByClassName("tab");
    for(const tab of tabs) {
        tab.style.display = "none";
    }

    // Show Target Tab:
    targetTab.style.display = "block";
}

function initTabs() {
    // Initalize Link Spaces:
    const linkspaces = document.getElementsByClassName("linkspace");
    for(const linkspace of linkspaces) {
        // Get Links in Linkspace:
        const links = linkspace.getElementsByClassName("link");
        
        // Style All Tab Links (Default Style):
        for(const link of links) {
            styleDefault(link);
        }

        // Style First Tab Link (Selected Style):
        styleSelected(links[0])
    }

    // Initalize Tab Spaces:
    const tabspaces = document.getElementsByClassName("tabspace");
    for(const tabspace of tabspaces) {
        // Get Tabs in Tabspace:
        const tabs = tabspace.getElementsByClassName("tab");
        
        // Hide All Tab (Default):
        for(const tab of tabs) {
            tab.style.display = "none";
        }

        // Show First Tab (Selected):
        tabs[0].style.display = "block";
    }
}